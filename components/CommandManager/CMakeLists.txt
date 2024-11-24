idf_component_register(
  SRCS 
    "CommandManager/CommandManager.cpp"
    "CommandManager/Commands.cpp"
  INCLUDE_DIRS
     "CommandManager"
  REQUIRES ProjectConfig cJSON
)