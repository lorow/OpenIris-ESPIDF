#include "wifiManager.hpp"

WiFiManager::state_e WiFiManager::_state = {state_e::NOT_INITIALIZED};

void event_handler(void *arg, esp_event_base_t event_base,
                   int32_t event_id, void *event_data)
{
  ESP_LOGI(WIFI_MAMANGER_TAG, "Trying to connect, got event: %d", (int)event_id);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK)
    {
      ESP_LOGI(WIFI_MAMANGER_TAG, "esp_wifi_connect() failed: %s", esp_err_to_name(err));
      // xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(WIFI_MAMANGER_TAG, "retry to connect to the AP");
    }
    else
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(WIFI_MAMANGER_TAG, "connect to the AP fail");
  }

  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(WIFI_MAMANGER_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void WiFiManager::SetCredentials(const char *ssid, const char *password)
{
  memcpy(_wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(_wifi_cfg.sta.ssid)));

  memcpy(_wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(_wifi_cfg.sta.password)));
}

void WiFiManager::Begin()
{
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t esp_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&esp_wifi_init_config));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  _wifi_cfg = {};
  _wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WEP;
  _wifi_cfg.sta.pmf_cfg.capable = true;
  _wifi_cfg.sta.pmf_cfg.required = false;

  // we try to connect with the hardcoded credentials first
  // todo, add support for more networks and setting up an AP
  this->SetCredentials(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSOWRD);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifi_cfg));

  _state = state_e::READY_TO_CONNECT;
  esp_wifi_start();
  ESP_LOGI(WIFI_MAMANGER_TAG, "wifi_init_sta finished.");

  _state = state_e::CONNECTING;
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
   * happened. */
  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(WIFI_MAMANGER_TAG, "connected to ap SSID:%s password:%s",
             _wifi_cfg.sta.ssid, _wifi_cfg.sta.password);
    _state = state_e::CONNECTED;
  }

  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGE(WIFI_MAMANGER_TAG, "Failed to connect to SSID:%s, password:%s",
             _wifi_cfg.sta.ssid, _wifi_cfg.sta.password);
    _state = state_e::ERROR;
  }
  else
  {
    ESP_LOGE(WIFI_MAMANGER_TAG, "UNEXPECTED EVENT");
  }
}
