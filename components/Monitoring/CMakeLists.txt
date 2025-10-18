idf_component_register(SRCS 
  "Monitoring/CurrentMonitor.cpp"
  "Monitoring/MonitoringManager.cpp"
  INCLUDE_DIRS "Monitoring"
  REQUIRES driver esp_adc Helpers
)
