idf_component_register(
  SRCS 
    "CommandManager/CommandManager.cpp"
    "CommandManager/commands/simple_commands.cpp"
    "CommandManager/commands/camera_commands.cpp"
    "CommandManager/commands/wifi_commands.cpp"
    "CommandManager/commands/config_commands.cpp"
    "CommandManager/commands/mdns_commands.cpp"
    "CommandManager/commands/device_commands.cpp"
  INCLUDE_DIRS
     "CommandManager"
     "CommandManager/commands"
  REQUIRES ProjectConfig cJSON CameraManager OpenIrisTasks
)