#include "wifiManager.hpp"

void WiFiManagerHelpers::event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
  ESP_LOGI(WIFI_MAMANGER_TAG, "Trying to connect, got event: %d", (int)event_id);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK)
    {
      ESP_LOGI(WIFI_MAMANGER_TAG, "esp_wifi_connect() failed: %s", esp_err_to_name(err));
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

WiFiManager::WiFiManager(std::shared_ptr<ProjectConfig> deviceConfig) : deviceConfig(deviceConfig) {}

void WiFiManager::SetCredentials(const char *ssid, const char *password)
{
  memcpy(_wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(_wifi_cfg.sta.ssid)));

  memcpy(_wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(_wifi_cfg.sta.password)));
}

void WiFiManager::ConnectWithHardcodedCredentials()
{
  this->SetCredentials(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSOWRD);
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifi_cfg));

  wifiStateManager.setState(WiFiState_e::WiFiState_ReadyToConect);
  esp_wifi_start();

  wifiStateManager.setState(WiFiState_e::WiFiState_Connecting);
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

    wifiStateManager.setState(WiFiState_e::WiFiState_Connected);
  }

  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGE(WIFI_MAMANGER_TAG, "Failed to connect to SSID:%s, password:%s",
             _wifi_cfg.sta.ssid, _wifi_cfg.sta.password);
    wifiStateManager.setState(WiFiState_e::WiFiState_Error);
  }
  else
  {
    ESP_LOGE(WIFI_MAMANGER_TAG, "UNEXPECTED EVENT");
  }
}

void WiFiManager::ConnectWithStoredCredentials()
{
  auto networks = this->deviceConfig->getWifiConfigs();
  for (auto network : networks)
  {
    xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
    this->SetCredentials(network.ssid.c_str(), network.password.c_str());

    // we need to update the config after every credentials change
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_wifi_cfg));

    wifiStateManager.setState(WiFiState_e::WiFiState_ReadyToConect);

    esp_wifi_start();
    wifiStateManager.setState(WiFiState_e::WiFiState_Connecting);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
      ESP_LOGI(WIFI_MAMANGER_TAG, "connected to ap SSID:%s password:%s",
               _wifi_cfg.sta.ssid, _wifi_cfg.sta.password);

      wifiStateManager.setState(WiFiState_e::WiFiState_Connected);
      return;
    }
    ESP_LOGE(WIFI_MAMANGER_TAG, "Failed to connect to SSID:%s, password:%s, trying next stored network",
             _wifi_cfg.sta.ssid, _wifi_cfg.sta.password);
  }

  wifiStateManager.setState(WiFiState_e::WiFiState_Error);
  ESP_LOGE(WIFI_MAMANGER_TAG, "Failed to connect to all saved networks");
}

void WiFiManager::SetupAccessPoint()
{
  ESP_LOGI(WIFI_MAMANGER_TAG, "Connection to stored credentials failed, starting AP");

  esp_netif_create_default_wifi_ap();
  wifi_init_config_t esp_wifi_ap_init_config = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&esp_wifi_ap_init_config));

  wifi_config_t ap_wifi_config = {
      .ap = {
          .ssid = CONFIG_AP_WIFI_SSID,
          .password = CONFIG_AP_WIFI_PASSWORD,
          .max_connection = 1,

      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(WIFI_MAMANGER_TAG, "AP started.");
  wifiStateManager.setState(WiFiState_e::WiFiState_ADHOC);
}

void WiFiManager::Begin()
{
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  auto netif = esp_netif_create_default_wifi_sta();

  wifi_init_config_t esp_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&esp_wifi_init_config));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &WiFiManagerHelpers::event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &WiFiManagerHelpers::event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  _wifi_cfg = {};
  _wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WEP;
  _wifi_cfg.sta.pmf_cfg.capable = true;
  _wifi_cfg.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  ESP_LOGI(WIFI_MAMANGER_TAG, "Beginning setup");
  bool hasHardcodedCredentials = strlen(CONFIG_WIFI_SSID) > 0;
  if (hasHardcodedCredentials)
  {
    ESP_LOGI(WIFI_MAMANGER_TAG, "Detected hardcoded credentials, trying them out");
    this->ConnectWithHardcodedCredentials();
  }

  if (wifiStateManager.getCurrentState() != WiFiState_e::WiFiState_Connected || !hasHardcodedCredentials)
  {
    ESP_LOGI(WIFI_MAMANGER_TAG, "Hardcoded credentials failed or missing, trying stored credentials");
    xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
    this->ConnectWithStoredCredentials();
  }

  if (wifiStateManager.getCurrentState() != WiFiState_e::WiFiState_Connected)
  {
    ESP_LOGI(WIFI_MAMANGER_TAG, "Stored netoworks failed or hardcoded credentials missing, starting AP");
    xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
    esp_netif_destroy(netif);
    this->SetupAccessPoint();
  }
}
