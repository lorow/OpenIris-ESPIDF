idf_component_register(SRCS "MDNSManager/MDNSManager.cpp"
  INCLUDE_DIRS "MDNSManager"
  REQUIRES mdns ProjectConfig StateManager
)