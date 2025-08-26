#!/usr/bin/env python3
"""
OpenIris Setup CLI Tool

This tool automatically discovers OpenIris devices via heartbeat,
allows WiFi configuration, and monitors the device logs.
"""

import re
import json
import time
import threading
import argparse
import sys
import string
from typing import Dict, List, Optional, Tuple
import serial
import serial.tools.list_ports
from dataclasses import dataclass


@dataclass
class WiFiNetwork:
    ssid: str
    channel: int
    rssi: int
    mac_address: str
    auth_mode: int
    
    @property
    def security_type(self) -> str:
        """Convert auth_mode to human readable string"""
        auth_modes = {
            0: "Open",
            1: "WEP", 
            2: "WPA PSK",
            3: "WPA2 PSK",
            4: "WPA WPA2 PSK",
            5: "WPA2 Enterprise",
            6: "WPA3 PSK",
            7: "WPA2 WPA3 PSK"
        }
        return auth_modes.get(self.auth_mode, f"Unknown ({self.auth_mode})")


class OpenIrisDevice:
    def __init__(self, port: str, serial_number: str, debug: bool = False):
        
        self.port = port
        self.serial_number = serial_number
        self.connection: Optional[serial.Serial] = None
        self.networks: List[WiFiNetwork] = []
        self.debug = debug
        
    def connect(self) -> bool:
        """Connect to the device"""
        try:
            self.connection = serial.Serial(
                port=self.port,
                baudrate=115200,
                timeout=1,
                write_timeout=1
            )
            print(f"✅ Connected to device {self.serial_number} on {self.port}")
            
            # Immediately send pause command to keep device in setup mode
            print("⏸️  Pausing device startup...")
            # Use shorter timeout for pause command since device is just starting up
            pause_response = self.send_command("pause", {"pause": True}, timeout=5)
            if "error" not in pause_response and pause_response.get("results"):
                print("✅ Device paused in setup mode")
            elif "error" in pause_response and pause_response["error"] == "Command timeout":
                # Even if we timeout, the command likely worked (as seen in logs)
                print("✅ Device pause command sent (startup logs may have obscured response)")
            else:
                print(f"⚠️  Pause status uncertain: {pause_response}")
            
            return True
        except Exception as e:
            print(f"❌ Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the device"""
        if self.connection and self.connection.is_open:
            # Optionally unpause the device before disconnecting
            print("Resuming device startup...")
            self.send_command("pause", {"pause": False})
            
            self.connection.close()
            print(f"🔌 Disconnected from {self.port}")
    
    def send_command(self, command: str, params: Dict = None, timeout: int = None) -> Dict:
        """Send a command to the device and wait for response"""
        if not self.connection or not self.connection.is_open:
            return {"error": "Not connected"}
        
        cmd_obj = {"commands": [{"command": command}]}
        if params:
            cmd_obj["commands"][0]["data"] = params
        
        cmd_json = json.dumps(cmd_obj) + '\n'
        try:
            # Clear any pending data
            self.connection.reset_input_buffer()
            
            # Send command
            print(f"📤 Sending: {cmd_json.strip()}")
            self.connection.write(cmd_json.encode())
            
            # For scan_networks command, handle special case
            if command == "scan_networks":
                # Use provided timeout or default to 30 seconds for scan
                scan_timeout = timeout if timeout is not None else 30
                return self._handle_scan_response(scan_timeout)
            
            # Wait for response (skip heartbeats and logs)
            start_time = time.time()
            response_buffer = ""
            
            # Use provided timeout or default to 15 seconds
            cmd_timeout = timeout if timeout is not None else 15
            while time.time() - start_time < cmd_timeout:
                try:
                    if self.connection.in_waiting:
                        data = self.connection.read(self.connection.in_waiting).decode('utf-8', errors='ignore')
                        response_buffer += data
                        
                        # Show raw data for debugging
                        if self.debug and data.strip():
                            print(f"📡 Raw: {repr(data)}")
                            print(f"📝 Buffer: {repr(response_buffer[-200:])}")
                        
                        # Remove ANSI escape sequences
                        clean_buffer = re.sub(r'\x1b\[[0-9;]*m', '', response_buffer)
                        clean_buffer = clean_buffer.replace('\r', '')
                        
                        # Look for JSON objects - handle both single-line and multi-line
                        # Try to find complete JSON objects
                        start_idx = clean_buffer.find('{')
                        while start_idx >= 0:
                            # Count braces to find complete JSON
                            brace_count = 0
                            end_idx = -1
                            
                            for i in range(start_idx, len(clean_buffer)):
                                if clean_buffer[i] == '{':
                                    brace_count += 1
                                elif clean_buffer[i] == '}':
                                    brace_count -= 1
                                    if brace_count == 0:
                                        end_idx = i + 1
                                        break
                            
                            if end_idx > start_idx:
                                json_str = clean_buffer[start_idx:end_idx]
                                
                                # Try to parse any complete JSON object
                                try:
                                    # Clean up the JSON
                                    clean_json = json_str.replace('\t', ' ').replace('\n', ' ').replace('\r', '')
                                    clean_json = re.sub(r'\s+', ' ', clean_json)
                                    
                                    response = json.loads(clean_json)
                                    
                                    # Return if this is a command response with results
                                    if "results" in response:
                                        return response
                                        
                                except json.JSONDecodeError:
                                    pass
                                
                                # Look for next JSON object
                                start_idx = clean_buffer.find('{', end_idx)
                            else:
                                # No complete JSON found yet
                                break
                    else:
                        time.sleep(0.1)
                except Exception as e:
                    print(f"⚠️ Exception: {e}")
                    continue
            
            return {"error": "Command timeout"}
        except Exception as e:
            return {"error": f"Communication error: {e}"}
    
    def _handle_scan_response(self, timeout: int = 30) -> Dict:
        """Handle scan_networks command response which outputs raw JSON first"""
        start_time = time.time()
        response_buffer = ""
        
        while time.time() - start_time < timeout:  # Configurable timeout for scan
            if self.connection.in_waiting:
                data = self.connection.read(self.connection.in_waiting).decode('utf-8', errors='ignore')
                response_buffer += data
                
                # Look for WiFi networks JSON directly (new format)
                # The scan command now outputs JSON directly followed by command result
                if '{"networks":[' in response_buffer:
                    import re
                    
                    # Look for the networks JSON pattern that appears first
                    networks_pattern = r'\{"networks":\[.*?\]\}'
                    matches = re.findall(networks_pattern, response_buffer, re.DOTALL)
                    
                    for match in matches:
                        try:
                            # Parse the networks JSON directly
                            networks_data = json.loads(match)
                            if "networks" in networks_data:
                                # Return in the expected format for compatibility
                                return {"results": [json.dumps({"result": match})]}
                        except json.JSONDecodeError:
                            continue
                
                # Also check if we have the command result indicating completion
                if '{"results":' in response_buffer and '"Networks scanned"' in response_buffer:
                    # We've received the completion message, parse any networks found
                    import re
                    networks_pattern = r'\{"networks":\[.*?\]\}'
                    matches = re.findall(networks_pattern, response_buffer, re.DOTALL)
                    
                    for match in matches:
                        try:
                            networks_data = json.loads(match)
                            if "networks" in networks_data:
                                return {"results": [json.dumps({"result": match})]}
                        except json.JSONDecodeError:
                            continue
                    
                    # If we get here, scan completed but no networks found
                    return {"results": [json.dumps({"result": '{"networks":[]}'})]}
            else:
                time.sleep(0.1)
        
        return {"error": "Scan timeout"}
    
    def scan_networks(self, timeout: int = 30) -> bool:
        """Scan for WiFi networks"""
        print(f"🔍 Scanning for WiFi networks (this may take up to {timeout} seconds)...")
        response = self.send_command("scan_networks", timeout=timeout)
        
        if "error" in response:
            print(f"❌ Scan failed: {response['error']}")
            return False
        
        try:
            # Parse the nested JSON response
            results = response.get("results", [])
            if not results:
                print("❌ No scan results received")
                return False
            
            # The result is JSON-encoded string inside the response
            result_data = json.loads(results[0])
            networks_data = json.loads(result_data["result"])
            
            self.networks = []
            channels_found = set()
            
            for net in networks_data.get("networks", []):
                network = WiFiNetwork(
                    ssid=net["ssid"],
                    channel=net["channel"],
                    rssi=net["rssi"],
                    mac_address=net["mac_address"],
                    auth_mode=net["auth_mode"]
                )
                self.networks.append(network)
                channels_found.add(net["channel"])
            
            # Sort networks by RSSI (strongest first)
            self.networks.sort(key=lambda x: x.rssi, reverse=True)
            
            print(f"✅ Found {len(self.networks)} networks on channels: {sorted(channels_found)}")
            return True
            
        except Exception as e:
            print(f"❌ Failed to parse scan results: {e}")
            return False
    
    def set_wifi(self, ssid: str, password: str) -> bool:
        """Configure WiFi credentials"""
        print(f"🔧 Setting WiFi credentials for '{ssid}'...")
        
        params = {
            "name": "main",
            "ssid": ssid,
            "password": password,
            "channel": 0,
            "power": 0
        }
        
        response = self.send_command("set_wifi", params)
        
        if "error" in response:
            print(f"❌ WiFi setup failed: {response['error']}")
            return False
        
        print("✅ WiFi credentials set successfully")
        return True

    def set_mdns_name(self, name: str) -> bool:
        """Configure the MDNS hostname"""

        print(f"🔧 Setting MDNS name to '{name}'...")
        print("    The device should be accessible under")
        print(f"http://{name}.local/")
        print("\n    Note, this will also modify the name of the UVC device")

        params = {
            "hostname": name
        }

        response = self.send_command("set_mdns", params)
        if "error" in response:
            print(f"❌ MDNS name setup failed: {response['error']}")
            return False

        print("✅ MDNS name set successfully")
        return True

    def get_wifi_status(self) -> Dict:
        """Get current WiFi connection status"""
        response = self.send_command("get_wifi_status")
        
        if "error" in response:
            print(f"❌ Failed to get WiFi status: {response['error']}")
            return {}
        
        try:
            # Parse the nested JSON response
            results = response.get("results", [])
            if results:
                result_data = json.loads(results[0])
                # The result is a JSON-encoded string, need to decode it
                status_json = result_data["result"]
                
                # First, unescape the JSON string properly
                # Replace escaped backslashes and quotes
                status_json = status_json.replace('\\\\', '\\')
                status_json = status_json.replace('\\"', '"')
                
                # Now parse the cleaned JSON
                status_data = json.loads(status_json)
                return status_data
        except Exception as e:
            print(f"❌ Failed to parse WiFi status: {e}")
            # Try to show raw response for debugging
            if "results" in response and response["results"]:
                print(f"📝 Raw result: {response['results'][0]}")
        
        return {}
    
    def connect_wifi(self) -> bool:
        """Attempt to connect to configured WiFi"""
        print("🔗 Attempting WiFi connection...")
        response = self.send_command("connect_wifi")
        
        if "error" in response:
            print(f"❌ WiFi connection failed: {response['error']}")
            return False
        
        print("✅ WiFi connection attempt started")
        return True
    
    def start_streaming(self) -> bool:
        """Start streaming mode"""
        print("🚀 Starting streaming mode...")
        response = self.send_command("start_streaming")
        
        if "error" in response:
            print(f"❌ Failed to start streaming: {response['error']}")
            return False
        
        print("✅ Streaming mode started")
        return True
    
    def switch_mode(self, mode: str) -> bool:
        """Switch device mode between WiFi, UVC, and Auto"""
        print(f"🔄 Switching device mode to '{mode}'...")
        
        params = {"mode": mode}
        response = self.send_command("switch_mode", params)
        
        if "error" in response:
            print(f"❌ Failed to switch mode: {response['error']}")
            return False
        
        print(f"✅ Device mode switched to '{mode}' successfully!")
        print("🔄 Please restart the device for changes to take effect")
        return True
    
    def get_device_mode(self) -> str:
        """Get current device mode"""
        response = self.send_command("get_device_mode")
        
        if "error" in response:
            print(f"❌ Failed to get device mode: {response['error']}")
            return "unknown"
        
        try:
            results = response.get("results", [])
            if results:
                result_data = json.loads(results[0])
                mode_data = json.loads(result_data["result"])
                return mode_data.get("mode", "unknown")
        except Exception as e:
            print(f"❌ Failed to parse mode response: {e}")
            return "unknown"
    
    def get_mdns_name(self) -> str:
        """Get the current MDNS name"""
        response = self.send_command("get_mdns_name")
        if "error" in response:
            print(f"❌ Failed to get device name: {response['error']}")
            return "unknown"

        try:
            results = response.get("results", [])
            if results:
                result_data = json.loads(results[0])
                name_data = json.loads(result_data["result"])
                return name_data.get("hostname", "unknown")
        except Exception as e:
            print(f"❌ Failed to parse name response: {e}")
            return "unknown"

    def set_led_duty_cycle(self, duty_cycle):
        """Sets the PWN duty cycle of the LED"""
        print(f"🌟 Setting LED duty cycle to {duty_cycle}%...")
        response = self.send_command("set_led_duty_cycle", {"dutyCycle": duty_cycle})
        if "error" in response:
            print(f"❌ Failed to set LED duty cycle: {response['error']}")
            return False

        print("✅ LED duty cycle set successfully")
        return True

    def get_led_duty_cycle(self) -> Optional[int]:
        """Get the current LED PWM duty cycle from the device"""
        response = self.send_command("get_led_duty_cycle")
        if "error" in response:
            print(f"❌ Failed to get LED duty cycle: {response['error']}")
            return None
        try:
            results = response.get("results", [])
            if results:
                result_data = json.loads(results[0])
                payload = result_data["result"]
                if isinstance(payload, str):
                    payload = json.loads(payload)
                return int(payload.get("led_external_pwm_duty_cycle"))
        except Exception as e:
            print(f"❌ Failed to parse LED duty cycle: {e}")
        return None

    def get_serial_info(self) -> Optional[Tuple[str, str]]:
        """Get device serial number and MAC address"""
        response = self.send_command("get_serial")
        if "error" in response:
            print(f"❌ Failed to get serial/MAC: {response['error']}")
            return None
        try:
            results = response.get("results", [])
            if results:
                result_data = json.loads(results[0])
                payload = result_data["result"]
                if isinstance(payload, str):
                    payload = json.loads(payload)
                serial = payload.get("serial")
                mac = payload.get("mac")
                return serial, mac
        except Exception as e:
            print(f"❌ Failed to parse serial/MAC: {e}")
        return None

    def monitor_logs(self):
        """Monitor device logs until interrupted"""
        print("📋 Monitoring device logs (Press Ctrl+C to exit)...")
        print("-" * 60)
        
        if not self.connection or not self.connection.is_open:
            print("❌ Not connected to device")
            return
        
        try:
            while True:
                try:
                    if self.connection.in_waiting > 0:
                        line = self.connection.readline().decode().strip()
                        if line:
                            # Skip JSON command responses, show raw logs
                            if not (line.startswith('{') and line.endswith('}')):
                                print(line)
                            elif "heartbeat" not in line:
                                # Show non-heartbeat JSON responses
                                print(f"📡 {line}")
                    else:
                        time.sleep(0.1)  # Small delay to prevent busy waiting
                except Exception:
                    continue
        except KeyboardInterrupt:
            print("\n🛑 Log monitoring stopped")


class OpenIrisDiscovery:
    def __init__(self):
        self.devices: Dict[str, OpenIrisDevice] = {}
        self.discovery_active = False
        
    def discover_devices(self, timeout: int = 3) -> List[OpenIrisDevice]:
        """Discover OpenIris devices via heartbeat - ultra-fast concurrent scanning"""
        print(f"⚡ Fast-scanning for OpenIris devices...")
        
        # Get all serial ports
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            print("❌ No serial ports found")
            return []
        
        # Prioritize likely ESP32 USB ports for faster detection
        priority_ports = []
        other_ports = []
        
        for port in ports:
            # Common ESP32 USB-to-serial descriptions
            desc_lower = (port.description or "").lower()
            # Include generic "USB Serial Device" which is common on Windows
            if any(keyword in desc_lower for keyword in 
                   ["cp210", "ch340", "ftdi", "esp32", "silicon labs", "usb-serial", "usb serial", "usb serial device"]):
                priority_ports.append(port)
            else:
                other_ports.append(port)
        
        # Check priority ports first, then others
        sorted_ports = priority_ports + other_ports
        
        if priority_ports:
            print(f"📡 Checking {len(sorted_ports)} ports ({len(priority_ports)} prioritized USB serial ports)...")
        else:
            print(f"📡 Checking {len(sorted_ports)} serial ports...")
        
        discovered = {}
        lock = threading.Lock()
        threads = []
        
        def check_port_fast(port_info):
            """Check a single port for OpenIris heartbeat - optimized for speed"""
            try:
                # Initial connection timeout - 500ms
                ser = serial.Serial(port_info.device, 115200, timeout=0.5)
                ser.reset_input_buffer()
                
                # Wait up to 2 seconds for heartbeat
                start_time = time.time()
                while time.time() - start_time < 2.0:
                    try:
                        # Read timeout - 200ms
                        ser.timeout = 10
                        if ser.in_waiting > 0:
                            line = ser.readline()
                            if line:
                                try:
                                    data = json.loads(line.decode().strip())
                                    if (data.get("heartbeat") == "openiris_setup_mode" and 
                                        "serial" in data):
                                        serial_num = data["serial"]
                                        with lock:
                                            if serial_num not in discovered:
                                                device = OpenIrisDevice(port_info.device, serial_num, debug=False)
                                                discovered[serial_num] = device
                                                print(f"💓 Found {serial_num} on {port_info.device}")
                                                # Return immediately to stop checking this port
                                                ser.close()
                                                return True
                                except (json.JSONDecodeError, UnicodeDecodeError):
                                    pass
                        else:
                            time.sleep(0.05)  # Very short sleep
                    except Exception:
                        pass
                
                ser.close()
            except Exception:
                # Port not available or not the right device
                pass
            return False
        
        # Start concurrent port checking
        for port in sorted_ports:
            thread = threading.Thread(target=check_port_fast, args=(port,))
            thread.daemon = True
            thread.start()
            threads.append(thread)
        
        # Wait for threads to complete or timeout
        timeout_time = time.time() + timeout
        for thread in threads:
            remaining = timeout_time - time.time()
            if remaining > 0:
                thread.join(timeout=remaining)
            
            # If we found at least one device, return immediately
            if discovered:
                break
        
        devices = list(discovered.values())
        
        if devices:
            print(f"✅ Found {len(devices)} OpenIris device(s)")
        else:
            print("❌ No OpenIris devices found in {:.1f} seconds".format(time.time() - (timeout_time - timeout)))
            print("💡 Device has 20-second setup window after power on")
        
        return devices
    
    def _check_port(self, port: str, discovered: Dict, timeout: int):
        """Check a single port for OpenIris heartbeat"""
        try:
            with serial.Serial(port, 115200, timeout=1) as ser:
                start_time = time.time()
                
                while time.time() - start_time < timeout:
                    try:
                        line = ser.readline().decode().strip()
                        if line:
                            try:
                                data = json.loads(line)
                                if (data.get("heartbeat") == "openiris_setup_mode" and 
                                    "serial" in data):
                                    
                                    serial_num = data["serial"]
                                    if serial_num not in discovered:
                                        discovered[serial_num] = OpenIrisDevice(port, serial_num, debug=False)
                                        print(f"💓 Found device {serial_num} on {port}")
                                    return
                            except json.JSONDecodeError:
                                continue
                    except Exception:
                        continue
        except Exception:
            # Port not available or not a serial device
            pass


def scan_networks(device: OpenIrisDevice, args = None):
    should_use_custom_timeout = input("Use custom scan timeout? (y/N): ").strip().lower()
    if should_use_custom_timeout == 'y':
        try:
            timeout = int(input("Enter timeout in seconds (5-120): "))
            if 5 <= timeout <= 120:
                device.scan_networks(timeout)
            else:
                print("❌ Timeout must be between 5 and 120 seconds")
                device.scan_networks(args.scan_timeout)
        except ValueError:
            print("❌ Invalid timeout, using default")
            device.scan_networks(args.scan_timeout)
    else:
        device.scan_networks(args.scan_timeout)


def configure_wifi(device: OpenIrisDevice, args = None):
    if not device.networks:
        print("❌ No networks available. Please scan first.")
        return

    display_networks(device)

    while True:
        net_choice = input("\nEnter network number (or 'back'): ").strip()
        if net_choice.lower() == 'back':
            break

        try:
            net_idx = int(net_choice) - 1
        except ValueError:
            print("❌ Please enter a number or 'back'")
            continue

        sorted_networks = sorted(device.networks, key=lambda x: x.rssi, reverse=True)

        if 0 <= net_idx < len(sorted_networks):
            selected_network = sorted_networks[net_idx]

            print(f"\n🔐 Selected: {selected_network.ssid}")
            print(f"Security: {selected_network.security_type}")

            if selected_network.auth_mode == 0:  # Open network
                password = ""
                print("🔓 Open network - no password required")
            else:
                password = input("Enter WiFi password: ")

            if device.set_wifi(selected_network.ssid, password):
                print("✅ WiFi configured successfully!")
                print("💡 Next steps:")
                print("   • Open WiFi menu to connect to WiFi (if needed)")
                print("   • Open WiFi menu to check WiFi status")
                print("   • Start streaming from the main menu when connected")
            break
        else:
            print("❌ Invalid network number")


def configure_mdns(device: OpenIrisDevice, args = None):
    current_name = device.get_mdns_name()
    print(f"\n📍 Current device name: {current_name} \n")
    print("💡 Please enter your preferred device name, your board will be accessible under http://<name>.local/")
    print("💡 Please avoid spaces and special characters")
    print("    To back out, enter `back`")
    print("\n    Note, this will also modify the name of the UVC device")

    while True:
        name_choice = input("\nDevice name: ").strip()
        if any(space in name_choice for space in string.whitespace):
            print("❌ Please avoid spaces and special characters")
        else:
            break

    if name_choice.lower() == "back":
        return

    if device.set_mdns_name(name_choice):
        print("✅ MDNS configured successfully!")


def display_networks(device: OpenIrisDevice, args = None):
    """Display available WiFi networks in a formatted table"""
    networks = device.networks

    if not networks:
        print("❌ No networks available")
        return
    
    print("\n📡 Available WiFi Networks:")
    print("-" * 85)
    print(f"{'#':<3} {'SSID':<32} {'Channel':<8} {'Signal':<20} {'Security':<15}")
    print("-" * 85)
    
    # Networks are already sorted by signal strength from scan_networks
    for i, network in enumerate(networks, 1):
        # Create signal strength visualization
        signal_bars = "▓" * min(5, max(0, (network.rssi + 100) // 10))
        signal_str = f"{signal_bars:<5} ({network.rssi} dBm)"
        
        # Format SSID (show hidden networks as <hidden>)
        ssid_display = network.ssid if network.ssid else "<hidden>"
        
        print(f"{i:<3} {ssid_display:<32} {network.channel:<8} {signal_str:<20} {network.security_type:<15}")
    
    print("-" * 85)
    
    # Show channel distribution
    channels = {}
    for net in networks:
        channels[net.channel] = channels.get(net.channel, 0) + 1
    
    print(f"\n📊 Channel distribution: ", end="")
    for ch in sorted(channels.keys()):
        print(f"Ch{ch}: {channels[ch]} networks  ", end="")
    print()


def check_wifi_status(device: OpenIrisDevice, args = None):
    status = device.get_wifi_status()
    if status:
        print(f"📶 WiFi Status: {status.get('status', 'unknown')}")
        print(f"📡 Networks configured: {status.get('networks_configured', 0)}")
        if status.get('ip_address'):
            print(f"🌐 IP Address: {status['ip_address']}")
    else:
        print("❌ Unable to get WiFi status")


def attempt_wifi_connection(device: OpenIrisDevice, args = None):
    device.connect_wifi()
    print("🕰️ Wait a few seconds then check status in the WiFi menu")


def start_streaming(device: OpenIrisDevice, args = None):
    device.start_streaming()
    print("🚀 Streaming started! Use 'Monitor logs' from the main menu.")


# ----- WiFi submenu -----
def wifi_auto_setup(device: OpenIrisDevice, args=None):
    print("\n⚙️  Automatic WiFi setup starting...")
    scan_timeout = getattr(args, "scan_timeout", 30) if args else 30

    # 1) Scan
    if not device.scan_networks(timeout=scan_timeout):
        print("❌ Auto-setup aborted: no networks found or scan failed")
        return

    # 2) Show networks (sorted strongest-first already)
    display_networks(device)

    # 3) Select a network (default strongest)
    choice = input("Select network number [default: 1] or 'back': ").strip()
    if choice.lower() == "back":
        return
    try:
        idx = int(choice) - 1 if choice else 0
    except ValueError:
        idx = 0

    sorted_networks = sorted(device.networks, key=lambda x: x.rssi, reverse=True)
    if not (0 <= idx < len(sorted_networks)):
        print("⚠️  Invalid selection, using strongest network")
        idx = 0

    selected = sorted_networks[idx]
    print(f"\n🔐 Selected: {selected.ssid if selected.ssid else '<hidden>'}")
    if selected.auth_mode == 0:
        password = ""
        print("🔓 Open network - no password required")
    else:
        password = input("Enter WiFi password: ")

    # 4) Configure WiFi
    if not device.set_wifi(selected.ssid, password):
        print("❌ Auto-setup aborted: failed to configure WiFi")
        return

    # 5) Connect
    if not device.connect_wifi():
        print("❌ Auto-setup aborted: failed to start WiFi connection")
        return

    # 6) Wait for IP / connected status
    print("⏳ Connecting to WiFi, waiting for IP...")
    start = time.time()
    timeout_s = 30
    ip = None
    last_status = None
    while time.time() - start < timeout_s:
        status = device.get_wifi_status()
        last_status = status
        ip = (status or {}).get("ip_address")
        if ip and ip not in ("0.0.0.0", "", None):
            break
        time.sleep(0.5)

    if ip and ip not in ("0.0.0.0", "", None):
        print(f"✅ Connected! IP Address: {ip}")
    else:
        print("⚠️  Connection not confirmed within timeout")
        if last_status:
            print(f"   Status: {last_status.get('status', 'unknown')}  |  IP: {last_status.get('ip_address', '-')}")


def wifi_menu(device: OpenIrisDevice, args=None):
    while True:
        print("\n📶 WiFi Settings:")
        print(f"{str(1):>2}  ⚙️  Automatic WiFi setup")
        print(f"{str(2):>2}  📁 Manual WiFi actions")
        print("back  Back")

        choice = input("\nSelect option (1-2 or 'back'): ").strip()
        if choice.lower() == "back":
            break

        if choice == "1":
            wifi_auto_setup(device, args)
        elif choice == "2":
            wifi_manual_menu(device, args)
        else:
            print("❌ Invalid option")


def wifi_manual_menu(device: OpenIrisDevice, args=None):
    while True:
        print("\n📁 WiFi Manual Actions:")
        print(f"{str(1):>2}  🔍 Scan for WiFi networks")
        print(f"{str(2):>2}  📡 Show available networks")
        print(f"{str(3):>2}  🔐 Configure WiFi")
        print(f"{str(4):>2}  🔗 Connect to WiFi")
        print(f"{str(5):>2}  🛰️  Check WiFi status")
        print("back  Back")

        choice = input("\nSelect option (1-5 or 'back'): ").strip()
        if choice.lower() == "back":
            break

        sub_map = {
            "1": scan_networks,
            "2": display_networks,
            "3": configure_wifi,
            "4": attempt_wifi_connection,
            "5": check_wifi_status,
        }

        handler = sub_map.get(choice)
        if not handler:
            print("❌ Invalid option")
            continue
        handler(device, args)


def switch_device_mode(device: OpenIrisDevice, args = None):
    current_mode = device.get_device_mode()
    print(f"\n📍 Current device mode: {current_mode}")
    print("\n🔄 Select new device mode:")
    print("1. WiFi - Stream over WiFi connection")
    print("2. UVC - Stream as USB webcam")
    print("3. Auto - Automatic mode selection")

    mode_choice = input("\nSelect mode (1-3): ").strip()

    if mode_choice == "1":
        device.switch_mode("wifi")
    elif mode_choice == "2":
        device.switch_mode("uvc")
    elif mode_choice == "3":
        device.switch_mode("auto")
    else:
        print("❌ Invalid mode selection")


def set_led_duty_cycle(device: OpenIrisDevice, args=None):
    # Show current duty cycle on entry
    current = device.get_led_duty_cycle()
    if current is not None:
        print(f"💡 Current LED duty cycle: {current}%")

    while True:
        input_data = input("Enter LED external PWM duty cycle (0-100) or `back` to exit: \n")
        if input_data.lower() == "back":
            break

        try:
            duty_cycle = int(input_data)
        except ValueError:
            print("❌ Invalid input. Please enter a number between 0 and 100.")

        if duty_cycle < 0 or duty_cycle > 100:
            print("❌ Duty cycle must be between 0 and 100.")
        else:
            # Apply immediately; stay in loop for further tweaks
            if device.set_led_duty_cycle(duty_cycle):
                # Read back and display current value using existing getter
                updated = device.get_led_duty_cycle()
                if updated is not None:
                    print(f"💡 Current LED duty cycle: {updated}%")
                else:
                    print("ℹ️  Duty cycle updated, but current value could not be read back.")


def monitor_logs(device: OpenIrisDevice, args = None):
    device.monitor_logs()


def get_led_duty_cycle(device: OpenIrisDevice, args=None):
    duty = device.get_led_duty_cycle()
    if duty is not None:
        print(f"💡 Current LED duty cycle: {duty}%")


def get_serial(device: OpenIrisDevice, args=None):
    info = device.get_serial_info()
    if info is not None:
        serial, mac = info
#        print(f"🔑 Serial: {serial}")
        print(f"🔗 MAC:    {mac}")


# ----- Aggregated GET: settings summary -----
def _probe_serial(device: OpenIrisDevice) -> Dict:
    info = device.get_serial_info()
    if info is None:
        return {"serial": None, "mac": None}
    serial, mac = info
    return {"serial": serial, "mac": mac}


def _probe_led_pwm(device: OpenIrisDevice) -> Dict:
    duty = device.get_led_duty_cycle()
    return {"led_external_pwm_duty_cycle": duty}


def _probe_mode(device: OpenIrisDevice) -> Dict:
    mode = device.get_device_mode()
    return {"mode": mode}


def _probe_wifi_status(device: OpenIrisDevice) -> Dict:
    # Returns dict as provided by device; pass through
    status = device.get_wifi_status() or {}
    return {"wifi_status": status}


def get_settings(device: OpenIrisDevice, args=None):
    print("\n🧩 Collecting device settings...\n")

    probes = [
        ("Identity", _probe_serial),
        ("LED", _probe_led_pwm),
        ("Mode", _probe_mode),
        ("WiFi", _probe_wifi_status),
    ]

    summary: Dict[str, Dict] = {}

    for label, probe in probes:
        try:
            data = probe(device)
            summary[label] = data
        except Exception as e:
            summary[label] = {"error": str(e)}

    # Pretty print summary
    # Identity
    ident = summary.get("Identity", {})
    serial = ident.get("serial")
    mac = ident.get("mac")
    if serial:
        print(f"🔑 Serial: {serial}")
    # if mac:
    #     print(f"🔗 MAC:    {mac}")
    if not serial and not mac:
        print("🔑 Serial/MAC: unavailable")

    # LED
    led = summary.get("LED", {})
    duty = led.get("led_external_pwm_duty_cycle")
    if duty is not None:
        print(f"💡 LED PWM Duty: {duty}%")
    else:
        print("💡 LED PWM Duty: unknown")

    # Mode
    mode = summary.get("Mode", {}).get("mode")
    print(f"🎚️  Mode: {mode if mode else 'unknown'}")

    # WiFi
    wifi = summary.get("WiFi", {}).get("wifi_status", {})
    if wifi:
        status = wifi.get("status", "unknown")
        ip = wifi.get("ip_address") or "-"
        configured = wifi.get("networks_configured", 0)
        print(f"📶 WiFi: {status}  |  IP: {ip}  |  Networks configured: {configured}")
    else:
        print("📶 WiFi: status unavailable")

    print("")


COMMANDS_MAP = {
    "1": wifi_menu,
    "2": configure_mdns,
    "3": configure_mdns,
    "4": start_streaming,
    "5": switch_device_mode,
    "6": set_led_duty_cycle,
    "7": monitor_logs,
    "8": get_settings,
}


def main():
    parser = argparse.ArgumentParser(description="OpenIris Setup CLI Tool")
    parser.add_argument("--timeout", type=int, default=3, 
                       help="Discovery timeout in seconds (default: 3)")
    parser.add_argument("--port", type=str, 
                       help="Skip discovery and connect directly to specified port")
    parser.add_argument("--scan-timeout", type=int, default=30,
                       help="WiFi scan timeout in seconds (default: 30)")
    parser.add_argument("--no-auto", action="store_true",
                       help="Don't auto-connect to first device found")
    parser.add_argument("--debug", action="store_true",
                       help="Show debug output including raw serial data")
    args = parser.parse_args()
    
    print("🔧 OpenIris Setup Tool")
    print("=" * 50)
    
    device = None
    
    try:
        if args.port:
            # Connect directly to specified port
            print(f"📡 Connecting directly to {args.port}...")
            device = OpenIrisDevice(args.port, "direct", debug=args.debug)
            if not device.connect():
                return 1
        else:
            # Fast device discovery
            discovery = OpenIrisDiscovery()
            devices = discovery.discover_devices(args.timeout)
            
            if not devices:
                print("\n❌ No OpenIris devices found automatically")
                print("\n💡 Troubleshooting:")
                print("   - Make sure device is connected via USB")
                print("   - Device must be powered on within last 20 seconds")
                print("   - Try specifying port manually with --port")
                
                # Show available ports to help user
                print("\n📋 Available serial ports:")
                all_ports = list(serial.tools.list_ports.comports())
                for p in all_ports:
                    print(f"   - {p.device}: {p.description}")
                
                # Offer manual port entry
                manual_port = input("\n🔌 Enter serial port manually (e.g. COM15, /dev/ttyUSB0) or press Enter to exit: ").strip()
                if manual_port:
                    device = OpenIrisDevice(manual_port, "manual", debug=args.debug)
                    if not device.connect():
                        return 1
                else:
                    return 1
            else:
                # Auto-connect to first device found (unless disabled)
                if len(devices) == 1 or not args.no_auto:
                    device = devices[0]
                    device.debug = args.debug  # Set debug mode
                    print(f"\n🎯 Auto-connecting to {device.serial_number}...")
                    if not device.connect():
                        return 1
                else:
                    # Multiple devices found with no-auto flag
                    print("\n🔢 Multiple devices found. Select one:")
                    for i, dev in enumerate(devices, 1):
                        print(f"   {i}. {dev.serial_number} on {dev.port}")
                    
                    while True:
                        try:
                            choice = int(input("\nEnter device number: ")) - 1
                            if 0 <= choice < len(devices):
                                device = devices[choice]
                                break
                            else:
                                print("❌ Invalid selection")
                        except ValueError:
                            print("❌ Please enter a number")
                    
                    # Connect to selected device
                    device.debug = args.debug  # Set debug mode
                    if not device.connect():
                        return 1
        
        # Main interaction loop
        while True:
            print("\n🔧 Setup Options:")
            print(f"{str(1):>2}  📶 WiFi settings")
            print(f"{str(2):>2}  🌐 Configure MDNS")
            print(f"{str(3):>2}  💻 Configure UVC Name")
            print(f"{str(4):>2}  🚀 Start streaming mode")
            print(f"{str(5):>2}  🔄 Switch device mode (WiFi/UVC/Auto)")
            print(f"{str(6):>2}  💡 Update PWM Duty Cycle")
            print(f"{str(7):>2}  📖 Monitor logs")
            print(f"{str(8):>2}  🧩 Get settings summary")
            print("exit  🚪 Exit")
            choice = input("\nSelect option (1-8): ").strip()

            if choice == "exit":
                break

            command_to_run = COMMANDS_MAP.get(choice, None)
            if not command_to_run:
                print("❌ Invalid option")
                continue

            command_to_run(device, args)

    except KeyboardInterrupt:
        print("\n🛑 Setup interrupted")
    
    finally:
        if device:
            device.disconnect()
    
    return 0


if __name__ == "__main__":
    sys.exit(main())