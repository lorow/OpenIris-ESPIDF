idf_component_register(SRCS "UVCStream/UVCStream.cpp"
  INCLUDE_DIRS "UVCStream"
  REQUIRES esp_timer esp32-camera StateManager usb_device_uvc CameraManager
)