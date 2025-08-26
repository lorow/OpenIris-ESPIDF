idf_component_register(SRCS "LEDManager/LEDManager.cpp"
  INCLUDE_DIRS "LEDManager"
  REQUIRES StateManager driver esp_driver_ledc Helpers ProjectConfig
)