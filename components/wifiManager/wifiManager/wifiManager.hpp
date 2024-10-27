#pragma once
#ifndef WIFIHANDLER_HPP
#define WIFIHANDLER_HPP

#include <string>
#include <cstring>
#include <algorithm>
#include <StateManager.hpp>
#include <ProjectConfig.hpp>

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY 3
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
static const char *WIFI_MAMANGER_TAG = "[WIFI_MANAGER]";

namespace WiFiManagerHelpers
{
  void event_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data);
}

class WiFiManager
{
private:
  uint8_t channel;
  ProjectConfig &deviceConfig;
  wifi_init_config_t _wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t _wifi_cfg = {};

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  int8_t power;

  void SetCredentials(const char *ssid, const char *password);
  void ConnectWithHardcodedCredentials();
  void ConnectWithStoredCredentials();
  void SetupAccessPoint();

public:
  WiFiManager(ProjectConfig &deviceConfig);
  void Begin();
};

#endif