#pragma once
#ifndef WIFIHANDLER_HPP
#define WIFIHANDLER_HPP

#include <string>
#include <cstring>
#include <algorithm>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY 3
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
static const char *WIFI_MAMANGER_TAG = "[WIFI_MANAGER]";

class WiFiManager
{
public:
public:
  enum class state_e
  {
    NOT_INITIALIZED,
    INITIALIZED,
    READY_TO_CONNECT,
    CONNECTING,
    WAITING_FOR_IP,
    CONNECTED,
    DISCONNECTED,
    ERROR
  };

private:
  uint8_t channel;
  wifi_init_config_t _wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t _wifi_cfg = {};

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  static state_e _state;

  int8_t power;
  bool _enable_adhoc;

  // static void event_handler(void *arg, esp_event_base_t event_base,
  //                           int32_t event_id, void *event_data);

public:
  void SetCredentials(const char *ssid, const char *password);
  constexpr const state_e &GetState(void) { return _state; }
  void Begin();
};

#endif