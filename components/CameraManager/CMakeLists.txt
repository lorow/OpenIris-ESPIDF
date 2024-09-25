idf_component_register(SRCS "CameraManager/CameraManager.cpp"
  INCLUDE_DIRS "CameraManager"
  REQUIRES esp32-camera StateManager ProjectConfig driver esp_driver_ledc esp_psram
)