import os
import difflib
import argparse
from typing import Dict, Optional, List

HEADER_COLOR = "\033[95m"
OKGREEN = "\033[92m"
WARNING = "\033[93m"
OKBLUE = "\033[94m"
ENDC = "\033[0m"

BOARDS_DIR_NAME = "boards"
SDKCONFIG_DEFAULTS_FILENAME = "sdkconfig.base_defaults"


def get_root_path() -> str:
    return os.path.split(os.path.dirname(os.path.realpath(__file__)))[0]


def get_boards_root() -> str:
    return os.path.join(get_root_path(), BOARDS_DIR_NAME)


def enumerate_board_configs() -> Dict[str, str]:
    """Walk the boards directory and build a mapping of board names to absolute file paths.

    Naming strategy:
      - Relative path from boards/ to file with path separators replaced by '_'.
      - If the last two path segments are identical (e.g. project_babble/project_babble) collapse to a single segment.
      - For facefocusvr eye boards we keep eye_L / eye_R suffix to distinguish configs even though WHO_AM_I is same.
    """
    boards_dir = get_boards_root()
    mapping: Dict[str, str] = {}
    if not os.path.isdir(boards_dir):
        return mapping
    for root, _dirs, files in os.walk(boards_dir):
        for f in files:
            if f == SDKCONFIG_DEFAULTS_FILENAME:
                continue
            rel_path = os.path.relpath(os.path.join(root, f), boards_dir)
            parts = rel_path.split(os.sep)
            if len(parts) >= 2 and parts[-1] == parts[-2]:  # collapse duplicate tail
                parts = parts[:-1]
            board_key = "_".join(parts)
            mapping[board_key] = os.path.join(root, f)
    return mapping


BOARD_CONFIGS = enumerate_board_configs()


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser()
    p.add_argument(
        "-b",
        "--board",
        help="Board name (run with --list to see options). Flexible: accepts path-like or partial if unique.",
    )
    p.add_argument(
        "--list", help="List discovered boards and exit", action="store_true"
    )
    p.add_argument("--dry-run", help="Dry run, won't modify files", action="store_true")
    p.add_argument(
        "--diff",
        help="Show the difference between base config and selected board",
        action="store_true",
    )
    p.add_argument("--ssid", help="Set the WiFi SSID", type=str, default="")
    p.add_argument("--password", help="Set the WiFi password", type=str, default="")
    p.add_argument("--clear-wifi", help="Clear WiFi credentials", action="store_true")
    return p


def list_boards():
    print("Discovered boards:")
    width = max((len(k) for k in BOARD_CONFIGS), default=0)
    for name, path in sorted(BOARD_CONFIGS.items()):
        print(f"  {name.ljust(width)}  ->  {os.path.relpath(path, get_root_path())}")


def _suggest_boards(partial: str) -> List[str]:
    if not partial:
        return []
    partial_low = partial.lower()
    contains = [b for b in BOARD_CONFIGS if partial_low in b.lower()]
    if contains:
        return contains[:10]
    # fallback to fuzzy matching using difflib
    choices = list(BOARD_CONFIGS.keys())
    return difflib.get_close_matches(partial, choices, n=5, cutoff=0.4)


def normalize_board_name(raw: Optional[str]) -> Optional[str]:
    if raw is None:
        return None
    candidate = raw.strip()
    if not candidate:
        return None
    candidate = candidate.replace("\\", "/").rstrip("/")
    # strip leading folders like tools/, boards/
    parts = [
        p
        for p in candidate.split("/")
        if p not in (".", "") and p not in ("tools", "boards")
    ]
    if parts:
        candidate = parts[-1] if len(parts) == 1 else "_".join(parts)
    candidate = candidate.replace("-", "_")
    # exact match
    if candidate in BOARD_CONFIGS:
        return candidate
    # try ending match
    endings = [b for b in BOARD_CONFIGS if b.endswith(candidate)]
    if len(endings) == 1:
        return endings[0]
    if len(endings) > 1:
        print(f"Ambiguous board '{raw}'. Could be: {', '.join(endings)}")
        return None
    # attempt case-insensitive
    lower_map = {b.lower(): b for b in BOARD_CONFIGS}
    if candidate.lower() in lower_map:
        return lower_map[candidate.lower()]
    return None


def get_main_config_path() -> str:
    return os.path.join(get_root_path(), "sdkconfig")


def get_board_config_path(board_key: str) -> str:
    return BOARD_CONFIGS[board_key]


def get_base_config_path() -> str:
    # base defaults moved under boards directory
    return os.path.join(get_boards_root(), SDKCONFIG_DEFAULTS_FILENAME)


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


def handle_wifi_config(_new_config: dict, _main_config: dict, _args) -> dict:
    if _args.ssid:
        _new_config["CONFIG_WIFI_SSID"] = f'"{_args.ssid}"'
        _new_config["CONFIG_WIFI_PASSWORD"] = f'"{_args.password}"'
    else:
        if "CONFIG_WIFI_SSID" in _main_config:
            _new_config["CONFIG_WIFI_SSID"] = _main_config["CONFIG_WIFI_SSID"]
        if "CONFIG_WIFI_PASSWORD" in _main_config:
            _new_config["CONFIG_WIFI_PASSWORD"] = _main_config["CONFIG_WIFI_PASSWORD"]

    if _args.clear_wifi:
        _new_config["CONFIG_WIFI_SSID"] = '""'
        _new_config["CONFIG_WIFI_PASSWORD"] = '""'
    return _new_config


def compute_diff(_parsed_base_config: dict, _parsed_board_config: dict) -> dict:
    _diff = {}
    for _key in _parsed_board_config:
        if _key not in _parsed_base_config:
            if _parsed_board_config[_key] != "":
                _diff[_key] = f"{OKGREEN}+{ENDC} {_parsed_board_config[_key]}"
        else:
            if _parsed_board_config[_key] != _parsed_base_config[_key]:
                _diff[_key] = (
                    f"{OKGREEN}{_parsed_base_config[_key]}{ENDC} -> {OKBLUE}{_parsed_board_config[_key]}{ENDC}"
                )
    return _diff


def main():
    parser = build_arg_parser()
    args = parser.parse_args()

    if args.list:
        list_boards()
        return

    board_input = args.board
    if not board_input:
        parser.error("--board is required (or use --list)")
    normalized = normalize_board_name(board_input)
    if not normalized:
        print(f"{WARNING}Unknown board '{board_input}'.")
        suggestions = _suggest_boards(board_input)
        if suggestions:
            print("Did you mean: " + ", ".join(suggestions))
        print("Use --list to see all boards.")
        raise SystemExit(2)

    if not os.path.isfile(get_base_config_path()):
        raise SystemExit(f"Base defaults file not found: {get_base_config_path()}")

    print(
        f"{OKGREEN}Switching configuration to board:{ENDC} {OKBLUE}{normalized}{ENDC}"
    )
    print(f"{OKGREEN}Using defaults from :{ENDC} {get_base_config_path()}")
    print(
        f"{OKGREEN}Using board config from :{ENDC} {get_board_config_path(normalized)}"
    )

    with open(get_main_config_path(), "r+") as main_config:
        parsed_main_config = parse_config(main_config)

    with (
        open(get_base_config_path(), "r") as base_config,
        open(get_board_config_path(normalized), "r") as board_config,
    ):
        parsed_base_config = parse_config(base_config)
        parsed_board_config = parse_config(board_config)

    new_board_config = {**parsed_base_config, **parsed_board_config}
    new_board_config = handle_wifi_config(new_board_config, parsed_main_config, args)

    if args.diff:
        print("---" * 5, f"{WARNING}DIFF{ENDC}", "---" * 5)
        diff = compute_diff(parsed_main_config, new_board_config)
        if not diff:
            print(
                f"{HEADER_COLOR}[DIFF]{ENDC} No changes between existing main config and {OKBLUE}{normalized}{ENDC}"
            )
        else:
            print(
                f"{HEADER_COLOR}[DIFF]{ENDC} Keys differing (main -> new {OKBLUE}{normalized}{ENDC}):"
            )
            for key in sorted(diff):
                print(f"{HEADER_COLOR}[DIFF]{ENDC} {key} : {diff[key]}")
        print("---" * 14)

    if not args.dry_run:
        print(f"{WARNING}Writing changes to main config file{ENDC}")
        with open(get_main_config_path(), "w") as main_config:
            for key, value in new_board_config.items():
                if value:
                    main_config.write(f"{key}={value}\n")
                else:
                    main_config.write(f"{key}\n")
    else:
        print(f"{WARNING}[DRY-RUN]{ENDC} Skipping writing to files")
    print(
        f"{OKGREEN}Done. ESP-IDF is setup to build for:{ENDC} {OKBLUE}{normalized}{ENDC}"
    )


if __name__ == "__main__":
    main()
