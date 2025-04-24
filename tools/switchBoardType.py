import os
import argparse

HEADER_COLOR = "\033[95m"
OKGREEN = '\033[92m'
WARNING = '\033[93m'
OKBLUE = '\033[94m'
ENDC = '\033[0m'

sdkconfig_defaults = "sdkconfig.base_defaults"
supported_boards = [
  "xiao-esp32s3",
  "project_babble"
]

parser = argparse.ArgumentParser()
parser.add_argument("-b", "--board", help="Board to switch to", choices=supported_boards)
parser.add_argument("--dry-run", help="Dry run, won't modify files", action="store_true", required=False)
parser.add_argument("--diff", help="Show the difference between base config and selected board", action="store_true", required=False)
parser.add_argument("--ssid", help="Set the SSID for the selected board", required=False, type=str, default="")
parser.add_argument("--password", help="Set the password For the provided network", required=False, type=str, default="")
parser.add_argument("--clear-wifi", help="Should we clear the wifi details", action="store_true", required=False)
args = parser.parse_args()


def get_root_path() -> str:
  return os.path.split(os.path.dirname(os.path.realpath(__file__)))[0]


def get_main_config_path() -> str:
  return os.path.join(get_root_path(), "sdkconfig")


def get_board_config_path() -> str:
  return os.path.join(get_root_path(), f"sdkconfig.board.{args.board}")


def get_base_config_path() -> str:
  return os.path.join(get_root_path(), sdkconfig_defaults)


def parse_config(config_file) -> dict:
  config = {}
  for line in config_file:
    line = line.strip().split("=")
    if len(line) == 2:
      config[line[0]] = line[1]
    else:
      # lines without value are usually comments, we're safe to store empty string there
      config[line[0]] = ""
  return config


def handle_wifi_config(_new_config: dict, _main_config: dict) -> dict:
  if args.ssid:
    _new_config["CONFIG_WIFI_SSID"] = f"\"{args.ssid}\""
    _new_config["CONFIG_WIFI_PASSWORD"] = f"\"{args.password}\""
  else:
    _new_config["CONFIG_WIFI_SSID"] = _main_config["CONFIG_WIFI_SSID"]
    _new_config["CONFIG_WIFI_PASSWORD"] = _main_config["CONFIG_WIFI_PASSWORD"]

  if args.clear_wifi:
    _new_config["CONFIG_WIFI_SSID"] = "\"\""
    _new_config["CONFIG_WIFI_PASSWORD"] = "\"\""
  return _new_config


def compute_diff(_parsed_base_config: dict, _parsed_board_config: dict) -> dict:
  _diff = {}
  for _key in _parsed_board_config:
    if _key not in _parsed_base_config:
      if _parsed_board_config[_key] != "":
        _diff[_key] = f"{OKGREEN}+{ENDC} {_parsed_board_config[_key]}"
    else:
      if _parsed_board_config[_key] != _parsed_base_config[_key]:
        _diff[_key] = f"{OKGREEN}{_parsed_base_config[_key]}{ENDC} -> {OKBLUE}{_parsed_board_config[_key]}{ENDC}"
  return _diff


print(f"{OKGREEN}Switching configuration to board:{ENDC} {OKBLUE}{args.board}{ENDC}")
print(f"{OKGREEN}Using defaults from :{ENDC} {get_base_config_path()}", )
print(f"{OKGREEN}Using board config from :{ENDC} {get_board_config_path()}")

main_config = open(get_main_config_path(), "r+")
parsed_main_config = parse_config(main_config)
main_config.close()

base_config = open(get_base_config_path(), "r")
board_config = open(get_board_config_path(), "r")

parsed_base_config = parse_config(base_config)
parsed_board_config = parse_config(board_config)

base_config.close()
board_config.close()

new_board_config = {**parsed_base_config, **parsed_board_config}
new_board_config = handle_wifi_config(new_board_config, parsed_main_config)

if args.diff:
  print("---"*5, f"{WARNING}DIFF{ENDC}", "---"*5)
  diff = compute_diff(parsed_main_config, new_board_config)
  if not diff:
    print(f"{HEADER_COLOR}[DIFF]{ENDC} Nothing has changed between the base config and {OKBLUE}{args.board}{ENDC} config")
  else:
    print(f"{HEADER_COLOR}[DIFF]{ENDC} The following keys have changed between the base config and {OKBLUE}{args.board}{ENDC} config:")
    for key in diff:
      print(f"{HEADER_COLOR}[DIFF]{ENDC} {key} : {diff[key]}")
  print("---"*14)

if not args.dry_run:
  # the main idea is to always replace the main config with the base config
  # and then add the board config on top of that, overriding where necessary.
  # This way we can have known working defaults safe from accidental modifications by espidf
  # with know working per-board config 
  # and a still modifiable sdkconfig for espidf

  print(f"{WARNING}Writing changes to main config file{ENDC}")
  with open(get_main_config_path(), "w") as main_config:
    for key, value in new_board_config.items():
      if value:
        main_config.write(f"{key}={value}\n")
      else:
        main_config.write(f"{key}\n")
else:
  print(f"{WARNING}[DRY-RUN]{ENDC} Skipping writing to files")
print(f"{OKGREEN}Done. ESP-IDF is setup to build for:{ENDC} {OKBLUE}{args.board}{ENDC}")
