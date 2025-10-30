set (
  requires
  esp_driver_uart
  CommandManager 
  ProjectConfig
)

if ("$ENV{IDF_ETARGET}" STREQUAL "esp32s3")
list(APPEND requires
            tinyusb
)
endif()

set (
  source_files
  "SerialManager/SerialManager.cpp"
)

if ("$ENV{IDF_ETARGET}" STREQUAL "esp32s3" OR "$ENV{IDF_ETARGET}" STREQUAL "esp32s2" )
list(APPEND source_files
            "SerialManager/SerialManager_esp32s3.cpp"
)
else()
list(APPEND source_files
            "SerialManager/SerialManager_esp32.cpp"
)
endif()

idf_component_register(SRCS ${source_files}
  INCLUDE_DIRS "SerialManager" 
  REQUIRES ${requires}
)