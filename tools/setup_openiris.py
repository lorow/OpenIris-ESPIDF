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


def get_wifi_status(device: OpenIrisDevice) -> dict:
    response = device.send_command("get_wifi_status")
    if has_command_failed(response):
        print(f"❌ Failed to get wifi status: {response['error']}")
        return {"wifi_status": "unknown"}

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
        ("LED", get_led_duty_cycle),
        ("Mode", get_device_mode),
        ("WiFi", get_wifi_status),
    ]

    summary: dict[str, dict] = {}

    for label, probe in probes:
        summary[label] = probe(device)

    print(f"🔑 Serial: {summary['Identity']}")
    print(f"💡 LED PWM Duty: {summary['LED']['duty_cycle']}%")
    print(f"🎚️  Mode: {summary['Mode']['mode']}")

    wifi = summary.get("WiFi", {}).get("wifi_status", {})
    status = wifi.get("status", "unknown")
    ip = wifi.get("ip_address") or "-"
    configured = wifi.get("networks_configured", 0)
    print(f"📶 WiFi: {status}  |  IP: {ip}  |  Networks configured: {configured}")


def handle_menu(menu_context: dict | None = None) -> str:
    menu = Menu("OpenIris Setup", menu_context)
    wifi_settings = SubMenu("📶 WiFi settings", menu_context, menu)
    wifi_settings.add_action("⚙️  Automatic WiFi setup", lambda: None)
    manual_wifi_actions = SubMenu(
        "📁 WiFi Manual Actions:",
        menu_context,
        wifi_settings,
    )

    # simple commands can just be functions, they will get passed some context to them by the menu
    manual_wifi_actions.add_action("🔍 Scan for WiFi networks", lambda: None)
    manual_wifi_actions.add_action("📡 Show available networks", lambda: None)
    manual_wifi_actions.add_action("🔐 Configure WiFi", lambda: None)
    manual_wifi_actions.add_action("🔗 Connect to WiFi", lambda: None)
    manual_wifi_actions.add_action("🛰️  Check WiFi status", lambda: None)

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

        try:
            handle_menu({"device": device, "args": args})
        except KeyboardInterrupt:
            print("\n🛑 Setup interrupted, disconnecting")

    return 0


if __name__ == "__main__":
    sys.exit(main())
