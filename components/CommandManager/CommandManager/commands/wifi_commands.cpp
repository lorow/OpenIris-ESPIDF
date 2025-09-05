#include "wifi_commands.hpp"
#include "esp_netif.h"
#include "sdkconfig.h"

std::optional<WifiPayload> parseSetWiFiCommandPayload(std::string_view jsonPayload)
{
  WifiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  const cJSON *networkName = cJSON_GetObjectItem(parsedJson, "name");
  const cJSON *ssidPtr = cJSON_GetObjectItem(parsedJson, "ssid");
  const cJSON *passwordPtr = cJSON_GetObjectItem(parsedJson, "password");
  const cJSON *channelPtr = cJSON_GetObjectItem(parsedJson, "channel");
  const cJSON *powerPtr = cJSON_GetObjectItem(parsedJson, "power");

  if (ssidPtr == nullptr)
  {
    // without an SSID we can't do anything
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  if (networkName == nullptr)
    payload.networkName = "main";
  else
    payload.networkName = std::string(networkName->valuestring);

  payload.ssid = std::string(ssidPtr->valuestring);

  if (passwordPtr == nullptr)
    payload.password = "";
  else
    payload.password = std::string(passwordPtr->valuestring);

  if (channelPtr == nullptr)
    payload.channel = 0;
  else
    payload.channel = channelPtr->valueint;

  if (powerPtr == nullptr)
    payload.power = 0;
  else
    payload.power = powerPtr->valueint;

  cJSON_Delete(parsedJson);
  return payload;
}

std::optional<deleteNetworkPayload> parseDeleteWifiCommandPayload(std::string_view jsonPayload)
{
  deleteNetworkPayload payload;
  auto *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  const auto *networkName = cJSON_GetObjectItem(parsedJson, "name");
  if (networkName == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.networkName = std::string(networkName->valuestring);

  cJSON_Delete(parsedJson);
  return payload;
}

std::optional<UpdateWifiPayload> parseUpdateWifiCommandPayload(std::string_view jsonPayload)
{
  UpdateWifiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  const cJSON *networkName = cJSON_GetObjectItem(parsedJson, "name");
  if (networkName == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.networkName = std::string(networkName->valuestring);

  const auto *ssidObject = cJSON_GetObjectItem(parsedJson, "ssid");
  const auto *passwordObject = cJSON_GetObjectItem(parsedJson, "password");
  const auto *channelObject = cJSON_GetObjectItem(parsedJson, "channel");
  const auto *powerObject = cJSON_GetObjectItem(parsedJson, "power");

  if (ssidObject != nullptr && (strcmp(ssidObject->valuestring, "") == 0))
  {
    // we need ssid to actually connect
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  if (ssidObject != nullptr)
    payload.ssid = std::string(ssidObject->valuestring);

  if (passwordObject != nullptr)
    payload.password = std::string(passwordObject->valuestring);

  if (channelObject != nullptr)
    payload.channel = channelObject->valueint;

  if (powerObject != nullptr)
    payload.power = powerObject->valueint;

  cJSON_Delete(parsedJson);
  return payload;
}

std::optional<UpdateAPWiFiPayload> parseUpdateAPWiFiCommandPayload(const std::string_view jsonPayload)
{
  UpdateAPWiFiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
  {
    return std::nullopt;
  }

  const cJSON *ssidObject = cJSON_GetObjectItem(parsedJson, "ssid");
  const cJSON *passwordObject = cJSON_GetObjectItem(parsedJson, "password");
  const cJSON *channelObject = cJSON_GetObjectItem(parsedJson, "channel");

  if (ssidObject != nullptr)
    payload.ssid = std::string(ssidObject->valuestring);

  if (passwordObject != nullptr)
    payload.password = std::string(passwordObject->valuestring);

  if (channelObject != nullptr)
    payload.channel = channelObject->valueint;

  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult setWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  const auto payload = parseSetWiFiCommandPayload(jsonPayload);

  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }
  const auto wifiConfig = payload.value();

  std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
  projectConfig->setWifiConfig(
      wifiConfig.networkName,
      wifiConfig.ssid,
      wifiConfig.password,
      wifiConfig.channel,
      wifiConfig.power);

  return CommandResult::getSuccessResult("Config updated");
}

CommandResult deleteWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  const auto payload = parseDeleteWifiCommandPayload(jsonPayload);
  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);

  projectConfig->deleteWifiConfig(payload.value().networkName);
  return CommandResult::getSuccessResult("Config updated");
}

CommandResult updateWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  const auto payload = parseUpdateWifiCommandPayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }

  auto updatedConfig = payload.value();

  auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
  auto storedNetworks = projectConfig->getWifiConfigs();
  if (const auto networkToUpdate = std::ranges::find_if(
    storedNetworks,
    [&](auto &network){ return network.name == updatedConfig.networkName; }
    );
      networkToUpdate != storedNetworks.end())
  {
    projectConfig->setWifiConfig(
        updatedConfig.networkName,
        updatedConfig.ssid.has_value() ? updatedConfig.ssid.value() : networkToUpdate->ssid,
        updatedConfig.password.has_value() ? updatedConfig.password.value() : networkToUpdate->password,
        updatedConfig.channel.has_value() ? updatedConfig.channel.value() : networkToUpdate->channel,
        updatedConfig.power.has_value() ? updatedConfig.power.value() : networkToUpdate->power);

    return CommandResult::getSuccessResult("Config updated");
  }
  else
    return CommandResult::getErrorResult("Requested network does not exist");
}

CommandResult updateAPWiFiCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  const auto payload = parseUpdateAPWiFiCommandPayload(jsonPayload);

  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  auto updatedConfig = payload.value();

  auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
  const auto previousAPConfig = projectConfig->getAPWifiConfig();

  projectConfig->setAPWifiConfig(
      updatedConfig.ssid.has_value() ? updatedConfig.ssid.value() : previousAPConfig.ssid,
      updatedConfig.password.has_value() ? updatedConfig.password.value() : previousAPConfig.password,
      updatedConfig.channel.has_value() ? updatedConfig.channel.value() : previousAPConfig.channel);

  return CommandResult::getSuccessResult("Config updated");
}

CommandResult getWiFiStatusCommand(std::shared_ptr<DependencyRegistry> registry) {
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  auto wifiManager = registry->resolve<WiFiManager>(DependencyType::wifi_manager);
  if (!wifiManager) {
    return CommandResult::getErrorResult("Not supported by current firmware");
  }
    auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    
    // Get current WiFi state
    auto wifiState = wifiManager->GetCurrentWiFiState();
    auto networks = projectConfig->getWifiConfigs();
    
    cJSON* statusJson = cJSON_CreateObject();
    
    // Add WiFi state
    const char* stateStr = "unknown";
    switch(wifiState) {
        case WiFiState_e::WiFiState_NotInitialized:
            stateStr = "not_initialized";
            break;
        case WiFiState_e::WiFiState_Initialized:
            stateStr = "initialized";
            break;
        case WiFiState_e::WiFiState_ReadyToConnect:
            stateStr = "ready";
            break;
        case WiFiState_e::WiFiState_Connecting:
            stateStr = "connecting";
            break;
        case WiFiState_e::WiFiState_WaitingForIp:
            stateStr = "waiting_for_ip";
            break;
        case WiFiState_e::WiFiState_Connected:
            stateStr = "connected";
            break;
        case WiFiState_e::WiFiState_Disconnected:
            stateStr = "disconnected";
            break;
        case WiFiState_e::WiFiState_Error:
            stateStr = "error";
            break;
    }
    cJSON_AddStringToObject(statusJson, "status", stateStr);
    cJSON_AddNumberToObject(statusJson, "networks_configured", networks.size());
    
    // Add IP info if connected
    if (wifiState == WiFiState_e::WiFiState_Connected) {
        // Get IP address from ESP32
        esp_netif_ip_info_t ip_info;
        esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            char ip_str[16];
            sprintf(ip_str, IPSTR, IP2STR(&ip_info.ip));
            cJSON_AddStringToObject(statusJson, "ip_address", ip_str);
        }
    }
    
    char* statusString = cJSON_PrintUnformatted(statusJson);
    std::string result(statusString);
    free(statusString);
    cJSON_Delete(statusJson);
    
    return CommandResult::getSuccessResult(result);
}

CommandResult connectWiFiCommand(std::shared_ptr<DependencyRegistry> registry) {
#if !CONFIG_GENERAL_ENABLE_WIRELESS
  return CommandResult::getErrorResult("Not supported by current firmware");
#endif
  auto wifiManager = registry->resolve<WiFiManager>(DependencyType::wifi_manager);
  if (!wifiManager) {
    return CommandResult::getErrorResult("Not supported by current firmware");
  }
    auto projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    
    auto networks = projectConfig->getWifiConfigs();
    if (networks.empty()) {
        return CommandResult::getErrorResult("No WiFi networks configured");
    }
    
    // Trigger WiFi connection attempt
    wifiManager->TryConnectToStoredNetworks();
    
    return CommandResult::getSuccessResult("WiFi connection attempt started");
}
