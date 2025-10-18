# /// script
# dependencies = [
#   "pyserial>=3.5",
# ]
# ///


import json
import time
import argparse
import sys
import serial
import string
from dataclasses import dataclass


def is_back(choice: str):
    return choice.lower() in ["back", "b", "exit"]


class SubMenu:
    def __init__(self, title, context: dict, parent_menu=None):
        self.title = title
        self.context = context
        self.items = []

        if parent_menu:
            parent_menu.add_submenu(self)

    def render(self, idx: int):
        print(f"{str(idx + 1):>2}  {self.title}")

    def add_submenu(self, submenu):
        self.items.append(submenu)

    def add_action(self, title, action):
        self.items.append((title, action))

    def validate_choice(self, choice: str):
        try:
            parsed_choice = int(choice)
            if parsed_choice >= 0 and parsed_choice <= len(self.items):
                return parsed_choice
        except ValueError:
            pass

        # we'll print the error regardless if it was an exception or wrong choice
        print("❌ Invalid choice")
        print("-" * 50)

    def show(self):
        while True:
            for idx, item in enumerate(self.items):
                if isinstance(item, SubMenu):
                    item.render(idx)
                else:
                    print(f"{str(idx + 1):>2}  {item[0]}")
            print("[Back] To go back")

            choice = input(">> ")
            if is_back(choice):
                break

            choice = self.validate_choice(choice)
            if not choice:
                continue

            selected_element = self.items[int(choice) - 1]
            if isinstance(selected_element, SubMenu):
                selected_element.show()
            else:
                print("-" * 50)
                selected_element[1](**self.context)
                print("-" * 50)


class Menu(SubMenu):
    def __init__(self, title, context=None, parent_menu=None):
        super().__init__(title, context, parent_menu)


class OpenIrisDevice:
    def __init__(self, port: str, debug: bool, debug_commands: bool):
        self.port = port
        self.debug = debug
        self.debug_commands = debug_commands
        self.connection: serial.Serial | None = None
        self.connected = False

    def __enter__(self):
        self.connected = self.__connect()
        return self

    def __exit__(self, type, value, traceback):
        self.__disconnect()

    def __connect(self) -> bool:
        print(f"📡 Connecting directly to {self.port}...")

        try:
            self.connection = serial.Serial(
                port=self.port, baudrate=115200, timeout=1, write_timeout=1
            )
            print(f"✅ Connected to the device on {self.port}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect to {self.port}: {e}")
            return False

    def __disconnect(self):
        if self.connection and self.connection.is_open:
            self.connection.close()
            print(f"🔌 Disconnected from {self.port}")

    def __check_if_response_is_complete(self, response) -> dict | None:
        try:
            return json.loads(response)
        except ValueError:
            return None

    def __read_response(self, timeout: int | None = None) -> dict | None:
        # we can try and retrieve the response now.
        # it should be more or less immediate, but some commands may take longer
        # so we gotta timeout
        timeout = timeout if timeout is not None else 15
        start_time = time.time()
        response_buffer = ""
        while time.time() - start_time < timeout:
            if self.connection.in_waiting:
                packet = self.connection.read_all().decode("utf-8", errors="ignore")
                if self.debug and packet.strip():
                    print(f"Received: {packet}")
                    print("-" * 10)
                    print(f"Current buffer: {response_buffer}")
                    print("-" * 10)

                # we can't rely on new lines to detect if we're done
                # nor can we assume that we're always gonna get valid json response
                # but we can assume that if we're to get a valid response, it's gonna be json
                # so we can start actually building the buffer only when
                # some part of the packet starts with "{", and start building from there
                # we can assume that no further data will be returned, so we can validate
                # right after receiving the last packet
                if (not response_buffer and "{" in packet) or response_buffer:
                    # assume we just started building the buffer and we've received the first packet

                    # alternative approach in case this doesn't work - we're always sending a valid json
                    # so we can start building the buffer from the first packet and keep trying to find the
                    # starting and ending brackets, extract that part and validate, if the message is complete, return
                    if not response_buffer:
                        starting_idx = packet.find("{")
                        response_buffer = packet[starting_idx:]
                    else:
                        response_buffer += packet

                    # and once we get something, we can validate if it's a valid json
                    if parsed_response := self.__check_if_response_is_complete(
                        response_buffer
                    ):
                        return parsed_response
            else:
                time.sleep(0.1)
        return None

    def is_connected(self) -> bool:
        return self.connected

    def send_command(
        self, command: str, params: dict | None = None, timeout: int | None = None
    ) -> dict:
        if not self.connection or not self.connection.is_open:
            return {"error": "Device Not Connected"}

        cmd_obj = {"commands": [{"command": command}]}
        if params:
            cmd_obj["commands"][0]["data"] = params

        # we're expecting the json string to end with a new line
        # to signify we've finished sending the command
        cmd_str = json.dumps(cmd_obj) + "\n"
        try:
            # clean it out first, just to be sure we're starting fresh
            self.connection.reset_input_buffer()
            if self.debug or self.debug_commands:
                print(f"Sending command: {cmd_str}")
            self.connection.write(cmd_str.encode())
            response = self.__read_response(timeout)

            if self.debug:
                print(f"Received response: {response}")

            return response or {"error": "Command timeout"}

        except Exception as e:
            return {"error": f"Communication error: {e}"}


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
            7: "WPA2 WPA3 PSK",
        }
        return auth_modes.get(self.auth_mode, f"Unknown ({self.auth_mode})")


class WiFiScanner:
    def __init__(self, device: OpenIrisDevice):
        self.device = device
        self.networks = []

    def scan_networks(self, timeout: int = 30):
        print(
            f"🔍 Scanning for WiFi networks (this may take up to {timeout} seconds)..."
        )
        response = self.device.send_command("scan_networks", timeout=timeout)
        if has_command_failed(response):
            print(f"❌ Scan failed: {response['error']}")
            return

        channels_found = set()
        networks = response["results"][0]["result"]["data"]["networks"]

        # after each scan, clear the network list to avoid duplication
        self.networks = []
        for net in networks:
            network = WiFiNetwork(
                ssid=net["ssid"],
                channel=net["channel"],
                rssi=net["rssi"],
                mac_address=net["mac_address"],
                auth_mode=net["auth_mode"],
            )
            self.networks.append(network)
            channels_found.add(net["channel"])

            # Sort networks by RSSI (strongest first)
        self.networks.sort(key=lambda x: x.rssi, reverse=True)
        print(
            f"✅ Found {len(self.networks)} networks on channels: {sorted(channels_found)}"
        )

    def get_networks(self) -> list:
        return self.networks


def has_command_failed(result) -> bool:
    return "error" in result or result["results"][0]["result"]["status"] != "success"


def get_device_mode(device: OpenIrisDevice) -> dict:
    command_result = device.send_command("get_device_mode")
    if has_command_failed(command_result):
        return {"mode": "unknown"}

    return {"mode": command_result["results"][0]["result"]["data"]["mode"].lower()}


def get_led_duty_cycle(device: OpenIrisDevice) -> dict:
    command_result = device.send_command("get_led_duty_cycle")
    if has_command_failed(command_result):
        print(f"❌ Failed to get LED duty cycle: {command_result['error']}")
        return {"duty_cycle": "unknown"}
    try:
        return {
            "duty_cycle": int(
                command_result["results"][0]["result"]["data"][
                    "led_external_pwm_duty_cycle"
                ]
            )
        }
    except ValueError as e:
        print(f"❌ Failed to parse LED duty cycle: {e}")
        return {"duty_cycle": "unknown"}


def get_mdns_name(device: OpenIrisDevice) -> dict:
    response = device.send_command("get_mdns_name")
    if "error" in response:
        print(f"❌ Failed to get device name: {response['error']}")
        return {"name": "unknown"}

    return {"name": response["results"][0]["result"]["data"]["hostname"]}


def get_serial_info(device: OpenIrisDevice) -> dict:
    response = device.send_command("get_serial")
    if has_command_failed(response):
        print(f"❌ Failed to get serial/MAC: {response['error']}")
        return {"serial": None, "mac": None}

    return {
        "serial": response["results"][0]["result"]["data"]["serial"],
        "mac": response["results"][0]["result"]["data"]["mac"],
    }

def get_device_info(device: OpenIrisDevice) -> dict:
    response = device.send_command("get_who_am_i")

    if has_command_failed(response):
        print(f"❌ Failed to get device info: {response['error']}")
        return {"who_am_i": None, "version": None}
    
    return {"who_am_i": response["results"][0]["result"]["data"]["who_am_i"], "version": response["results"][0]["result"]["data"]["version"]}


def get_wifi_status(device: OpenIrisDevice) -> dict:
    response = device.send_command("get_wifi_status")
    if has_command_failed(response):
        print(f"❌ Failed to get wifi status: {response['error']}")
        return {"wifi_status": {"status": "unknown"}}

    return {"wifi_status": response["results"][0]["result"]["data"]}


def configure_device_name(device: OpenIrisDevice, *args, **kwargs):
    current_name = get_mdns_name(device)
    print(f"\n📍 Current device name: {current_name['name']} \n")
    print(
        "💡 Please enter your preferred device name, your board will be accessible under http://<name>.local/"
    )
    print("💡 Please avoid spaces and special characters")
    print("    To back out, enter `back`")
    print("\n    Note, this will also modify the name of the UVC device")

    while True:
        name_choice = input("\nDevice name: ").strip()
        if any(space in name_choice for space in string.whitespace):
            print("❌ Please avoid spaces and special characters")
        else:
            break

    if is_back(name_choice):
        return

    response = device.send_command("set_mdns", {"hostname": name_choice})
    if "error" in response:
        print(f"❌ MDNS name setup failed: {response['error']}")
        return

    print("✅ MDNS name set successfully")


def start_streaming(device: OpenIrisDevice, *args, **kwargs):
    print("🚀 Starting streaming mode...")
    response = device.send_command("start_streaming")

    if "error" in response:
        print(f"❌ Failed to start streaming: {response['error']}")
        return

    print("✅ Streaming mode started")


def switch_device_mode_command(device: OpenIrisDevice, *args, **kwargs):
    modes = ["wifi", "uvc", "auto"]
    current_mode = get_device_mode(device)["mode"]
    print(f"\n📍 Current device mode: {current_mode}")
    print("\n🔄 Select new device mode:")
    print("1. WiFi - Stream over WiFi connection")
    print("2. UVC - Stream as USB webcam")
    print("3. Auto - Automatic mode selection")
    print("Back - Return to main menu")

    mode_choice = input("\nSelect mode (1-3): ").strip()
    if is_back(mode_choice):
        return

    try:
        mode = modes[int(mode_choice) - 1]
    except ValueError:
        print("❌ Invalid mode selection")
        return

    command_result = device.send_command("switch_mode", {"mode": mode})
    if "error" in command_result:
        print(f"❌ Failed to switch mode: {command_result['error']}")
        return

    print(f"✅ Device mode switched to '{mode}' successfully!")
    print("🔄 Please restart the device for changes to take effect")


def set_led_duty_cycle(device: OpenIrisDevice, *args, **kwargs):
    current_duty_cycle = get_led_duty_cycle(device)["duty_cycle"]
    print(f"💡 Current LED duty cycle: {current_duty_cycle}%")

    while True:
        desired_pwd = input(
            "Enter LED external PWM duty cycle (0-100) or `back` to exit: \n>> "
        )
        if is_back(desired_pwd):
            break

        try:
            duty_cycle = int(desired_pwd)
        except ValueError:
            print("❌ Invalid input. Please enter a number between 0 and 100.")

        if duty_cycle < 0 or duty_cycle > 100:
            print("❌ Duty cycle must be between 0 and 100.")
        else:
            response = device.send_command(
                "set_led_duty_cycle", {"dutyCycle": duty_cycle}
            )
            if has_command_failed(response):
                print(f"❌ Failed to set LED duty cycle: {response['error']}")
                return False

            print("✅ LED duty cycle set successfully")
            updated = get_led_duty_cycle(device)["duty_cycle"]
            print(f"💡 Current LED duty cycle: {updated}%")


def get_settings_summary(device: OpenIrisDevice, *args, **kwargs):
    print("🧩 Collecting device settings...\n")

    probes = [
        ("Identity", get_serial_info),
        ("Info", get_device_info),
        ("LED", get_led_duty_cycle),
        ("Mode", get_device_mode),
        ("WiFi", get_wifi_status),
    ]

    summary: dict[str, dict] = {}

    for label, probe in probes:
        summary[label] = probe(device)

    print(f"🔑 Serial: {summary['Identity']}")
    print(f"💡 LED PWM Duty: {summary['LED']['duty_cycle']}%")
    print(f"🎚️ Mode: {summary['Mode']['mode']}")
    
    info = summary.get("Info", {})
    who = info.get("who_am_i")
    ver = info.get("version")
    if who:
        print(f"🏷️  Device: {who}")
    if ver:
        print(f"🧭 Version: {ver}")


    wifi = summary.get("WiFi", {}).get("wifi_status", {})
    status = wifi.get("status", "unknown")
    ip = wifi.get("ip_address") or "-"
    configured = wifi.get("networks_configured", 0)
    print(f"📶 WiFi: {status}  |  IP: {ip}  |  Networks configured: {configured}")


def scan_networks(wifi_scanner: WiFiScanner, *args, **kwargs):
    use_custom_timeout = (
        input("Should we use a custom scan timeout? (y/n)\n>> ").strip().lower() == "y"
    )
    if use_custom_timeout:
        timeout = input("Enter timeout in seconds (5-120) or back to go back\n>> ")
        if is_back(timeout):
            return

        try:
            timeout = int(timeout)
            if 5 <= timeout <= 120:
                print(
                    f"🔍 Scanning for WiFi networks (this may take up to {timeout} seconds)..."
                )
                wifi_scanner.scan_networks(timeout)
            else:
                print("❌ Timeout must be between 5 and 120 seconds, using default")
                wifi_scanner.scan_networks()
        except ValueError:
            print("❌ Invalid timeout")
    else:
        wifi_scanner.scan_networks()


def display_networks(wifi_scanner: WiFiScanner, *args, **kwargs):
    networks = wifi_scanner.get_networks()
    if not networks:
        print("❌ No networks found, please scan first")
        return

    print("\n📡 Available WiFi Networks:")
    print("-" * 85)
    print(f"{'#':<3} {'SSID':<32} {'Channel':<8} {'Signal':<20} {'Security':<15}")
    print("-" * 85)

    channels = {}
    for idx, network in enumerate(networks, 1):
        channels[network.channel] = channels.get(network.channel, 0) + 1

        signal_bars = "▓" * min(5, max(0, (network.rssi + 100) // 10))
        signal_str = f"{signal_bars:<5} ({network.rssi} dBm)"
        ssid_display = network.ssid if network.ssid else "<hidden>"

        print(
            f"{idx:<3} {ssid_display:<32} {network.channel:<8} {signal_str:<20} {network.security_type:<15}"
        )

    print("-" * 85)

    print("\n📊 Channel distribution: ", end="")
    for channel in sorted(channels.keys()):
        print(f"Ch{channel}: {channels[channel]} networks  ", end="")


def configure_wifi(device: OpenIrisDevice, wifi_scanner: WiFiScanner, *args, **kwargs):
    networks = wifi_scanner.get_networks()
    if not networks:
        print("❌ No networks available. Please scan first.")
        return

    display_networks(wifi_scanner, *args, **kwargs)

    while True:
        net_choice = input("\nEnter network number (or 'back'): ").strip()
        if is_back(net_choice):
            break

        try:
            net_idx = int(net_choice) - 1
        except ValueError:
            print("❌ Please enter a number or 'back'")
            continue

        sorted_networks = wifi_scanner.get_networks()
        if 0 <= net_idx < len(sorted_networks):
            selected_network = sorted_networks[net_idx]
            ssid = selected_network.ssid

            print(f"\n🔐 Selected: {ssid}")
            print(f"Security: {selected_network.security_type}")

            if selected_network.auth_mode == 0:  # Open network
                password = ""
                print("🔓 Open network - no password required")
            else:
                password = input("Enter WiFi password (or 'back'): ")
                if is_back(password):
                    break

            print(f"🔧 Setting WiFi credentials for '{ssid}'...")

            params = {
                "name": "main",
                "ssid": ssid,
                "password": password,
                "channel": 0,
                "power": 0,
            }

            response = device.send_command("set_wifi", params)
            if has_command_failed(response):
                print(f"❌ WiFi setup failed: {response['error']}")
                break

            print("✅ WiFi configured successfully!")
            print("💡 Next steps:")
            print("   • Open WiFi menu to connect to WiFi (if needed)")
            print("   • Open WiFi menu to check WiFi status")
            print("   • Start streaming from the main menu when connected")
            break
        else:
            print("❌ Invalid network number")


def automatic_wifi_configuration(
    device: OpenIrisDevice, wifi_scanner: WiFiScanner, *args, **kwargs
):
    print("\n⚙️  Automatic WiFi setup starting...")
    scan_networks(wifi_scanner, *args, **kwargs)
    configure_wifi(device, wifi_scanner, *args, **kwargs)
    attempt_wifi_connection(device)

    print("⏳ Connecting to WiFi, waiting for IP...")
    start = time.time()
    timeout_s = 30
    ip = None
    last_status = None
    while time.time() - start < timeout_s:
        status = get_wifi_status(device).get("wifi_status", {})
        last_status = status
        ip = status.get("ip_address")
        if ip and ip not in ("0.0.0.0", "", None):
            break
        time.sleep(0.5)

    if ip and ip not in ("0.0.0.0", "", None):
        print(f"✅ Connected! IP Address: {ip}")
    else:
        print("⚠️  Connection not confirmed within timeout")
        if last_status:
            print(
                f"   Status: {last_status.get('status', 'unknown')}  |  IP: {last_status.get('ip_address', '-')}"
            )


def attempt_wifi_connection(device: OpenIrisDevice, *args, **kwargs):
    print("🔗 Attempting WiFi connection...")
    response = device.send_command("connect_wifi")
    if has_command_failed(response):
        print(f"❌ WiFi connection failed: {response['error']}")
        return

    print("✅ WiFi connection attempt started")


def check_wifi_status(device: OpenIrisDevice, *args, **kwargs):
    status = get_wifi_status(device).get("wifi_status")
    print(f"📶 WiFi Status: {status.get('status', 'Unknown')}")
    if ip_address := status.get("ip_address"):
        print(f"🌐 IP Address: {ip_address}")


def handle_menu(menu_context: dict | None = None) -> str:
    menu = Menu("OpenIris Setup", menu_context)
    wifi_settings = SubMenu("📶 WiFi settings", menu_context, menu)
    wifi_settings.add_action("⚙️  Automatic WiFi setup", automatic_wifi_configuration)
    manual_wifi_actions = SubMenu(
        "📁 WiFi Manual Actions:",
        menu_context,
        wifi_settings,
    )

    manual_wifi_actions.add_action("🔍 Scan for WiFi networks", scan_networks)
    manual_wifi_actions.add_action("📡 Show available networks", display_networks)
    manual_wifi_actions.add_action("🔐 Configure WiFi", configure_wifi)
    manual_wifi_actions.add_action("🔗 Connect to WiFi", attempt_wifi_connection)
    manual_wifi_actions.add_action("🛰️  Check WiFi status", check_wifi_status)

    menu.add_action("🌐 Configure MDNS", configure_device_name)
    menu.add_action("💻 Configure UVC Name", configure_device_name)
    menu.add_action("🚀 Start streaming mode", start_streaming)
    menu.add_action("🔄 Switch device mode (WiFi/UVC/Auto)", switch_device_mode_command)
    menu.add_action("💡 Update PWM Duty Cycle", set_led_duty_cycle)
    menu.add_action("🧩 Get settings summary", get_settings_summary)
    menu.show()


def valid_port(port: str):
    if not port.startswith("COM"):
        raise argparse.ArgumentTypeError("Invalid port name. We only support COM ports")
    return port


def main():
    parser = argparse.ArgumentParser(description="OpenIris CLI Setup Tool")
    parser.add_argument(
        "--port",
        type=valid_port,
        help="Serial port to connect to [COM4, COM3, etc]",
        required=True,
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Show debug output including raw serial data",
    )
    parser.add_argument(
        "--show-commands",
        action="store_true",
        help="Debug mode, but will show only sent commands",
    )
    args = parser.parse_args()

    print("🔧 OpenIris Setup Tool")
    print("=" * 50)

    with OpenIrisDevice(args.port, args.debug, args.show_commands) as device:
        if not device.is_connected():
            return 1

        wifi_scanner = WiFiScanner(device)
        try:
            handle_menu({"device": device, "args": args, "wifi_scanner": wifi_scanner})
        except KeyboardInterrupt:
            print("\n🛑 Setup interrupted, disconnecting")

    return 0


if __name__ == "__main__":
    sys.exit(main())
