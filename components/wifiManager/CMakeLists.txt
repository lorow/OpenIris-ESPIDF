idf_component_register(SRCS "wifiManager/wifiManager.cpp"
  INCLUDE_DIRS "wifiManager"
  REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip StateManager ProjectConfig
)