import serial
import time
import json
from typing import List, Dict

class ESPWiFiScanner:
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None

    def connect(self) -> bool:
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1
            )
            return True
        except serial.SerialException as e:
            print(f"Error connecting to ESP32: {e}")
            return False

    def scan_networks(self, timeout_seconds: int = 30) -> List[Dict]:
        if not self.serial:
            print("Not connected to ESP32")
            return []

        self.serial.reset_input_buffer()
        
        command = '{"commands":[{"command":"scan_networks","data":{}}]}\n'
        self.serial.write(command.encode())
        
        timeout_start = time.time()
        response_buffer = ""
        
        while time.time() - timeout_start < timeout_seconds:
            if self.serial.in_waiting:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
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
                                return networks_data["networks"]
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
                                return networks_data["networks"]
                        except json.JSONDecodeError:
                            continue
                    
                    # If we get here, scan completed but no networks found
                    return []
            else:
                time.sleep(0.1)
        
        print("Failed to receive clean JSON response. Raw data:")
        print("=" * 50)
        print(response_buffer)
        print("=" * 50)
        return []

    def close(self):
        if self.serial:
            self.serial.close()

def main():
    import sys
    import argparse
    
    parser = argparse.ArgumentParser(description='ESP32 WiFi Scanner')
    parser.add_argument('port', nargs='?', default='COM15', help='Serial port (default: COM9)')
    parser.add_argument('-t', '--timeout', type=int, default=30, 
                        help='Scan timeout in seconds (default: 30)')
    args = parser.parse_args()
    
    scanner = ESPWiFiScanner(args.port)
    
    if scanner.connect():
        print(f"Connected to ESP32 on {args.port}")
        print(f"Scanning for WiFi networks (timeout: {args.timeout} seconds)...")
        start_time = time.time()
        networks = scanner.scan_networks(args.timeout)
        scan_time = time.time() - start_time
        
        if networks:
            # Sort by RSSI (strongest first)
            networks.sort(key=lambda x: x.get('rssi', -100), reverse=True)
            
            print(f"\n✅ Found {len(networks)} WiFi Networks in {scan_time:.1f} seconds:")
            print("{:<32} | {:<7} | {:<15} | {:<17} | {:<9}".format(
                "SSID", "Channel", "Signal", "MAC Address", "Security"
            ))
            print("-" * 85)
            
            # Track channels found
            channels_found = set()
            auth_modes = {
                0: "Open",
                1: "WEP",
                2: "WPA-PSK",
                3: "WPA2-PSK", 
                4: "WPA/WPA2",
                5: "WPA2-Enterprise",
                6: "WPA3-PSK",
                7: "WPA2/WPA3",
                8: "WAPI-PSK"
            }
            
            for network in networks:
                ssid = network.get('ssid', '')
                if not ssid:
                    ssid = "<hidden>"
                
                channel = network.get('channel', 0)
                channels_found.add(channel)
                
                # Create signal strength visualization
                rssi = network.get('rssi', -100)
                signal_bars = "▓" * min(5, max(0, (rssi + 100) // 10))
                signal_str = f"{signal_bars:<5} ({rssi} dBm)"
                
                auth_mode = network.get('auth_mode', 0)
                security = auth_modes.get(auth_mode, f"Type {auth_mode}")
                    
                print("{:<32} | {:<7} | {:<15} | {:<17} | {:<9}".format(
                    ssid[:32],  # Truncate long SSIDs
                    channel,
                    signal_str,
                    network.get('mac_address', '?'),
                    security
                ))
            
            # Show channel distribution
            print(f"\n📊 Channels detected: {sorted(channels_found)}")
            channel_counts = {}
            for net in networks:
                ch = net.get('channel', 0)
                channel_counts[ch] = channel_counts.get(ch, 0) + 1
            
            print("📊 Channel distribution: ", end="")
            for ch in sorted(channel_counts.keys()):
                print(f"Ch{ch}: {channel_counts[ch]} networks  ", end="")
            print()
            
        else:
            print("❌ No networks found or scan failed")
            
        scanner.close()
    else:
        print(f"Failed to connect to ESP32 on {args.port}")

if __name__ == "__main__":
    main()