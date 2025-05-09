idf_component_register(SRCS "SerialManager/SerialManager.cpp"
  INCLUDE_DIRS "SerialManager"
  REQUIRES  esp_driver_uart CommandManager
)