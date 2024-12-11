#include "wifi_commands.hpp"

std::optional<WifiPayload> setWiFiCommand::parsePayload(std::string_view jsonPayload)
{
  WifiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *networkName = cJSON_GetObjectItem(parsedJson, "name");
  cJSON *ssidPtr = cJSON_GetObjectItem(parsedJson, "ssid");
  cJSON *passwordPtr = cJSON_GetObjectItem(parsedJson, "password");
  cJSON *channelPtr = cJSON_GetObjectItem(parsedJson, "channel");
  cJSON *powerPtr = cJSON_GetObjectItem(parsedJson, "power");

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

CommandResult setWiFiCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }
  auto wifiConfig = payload.value();
  projectConfig->setWifiConfig(
      wifiConfig.networkName,
      wifiConfig.ssid,
      wifiConfig.password,
      wifiConfig.channel,
      wifiConfig.power);

  return CommandResult::getSuccessResult("Config updated");
}

std::optional<deleteNetworkPayload> deleteWifiCommand::parsePayload(std::string_view jsonPayload)
{
  deleteNetworkPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *networkName = cJSON_GetObjectItem(parsedJson, "name");
  if (networkName == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.networkName = std::string(networkName->valuestring);

  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult deleteWifiCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  projectConfig->deleteWifiConfig(payload.value().networkName);
  return CommandResult::getSuccessResult("Config updated");
}

std::optional<UpdateWifiPayload> updateWifiCommand::parsePayload(std::string_view jsonPayload)
{
  UpdateWifiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *networkName = cJSON_GetObjectItem(parsedJson, "name");
  if (networkName == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.networkName = std::string(networkName->valuestring);

  cJSON *ssidObject = cJSON_GetObjectItem(parsedJson, "ssid");
  cJSON *passwordObject = cJSON_GetObjectItem(parsedJson, "password");
  cJSON *channelObject = cJSON_GetObjectItem(parsedJson, "channel");
  cJSON *powerObject = cJSON_GetObjectItem(parsedJson, "power");

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

CommandResult updateWifiCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }

  auto updatedConfig = payload.value();
  auto storedNetworks = projectConfig->getWifiConfigs();
  if (auto networkToUpdate = std::find_if(
          storedNetworks.begin(),
          storedNetworks.end(),
          [&](auto &network)
          { return network.name == updatedConfig.networkName; });
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

std::optional<UpdateAPWiFiPayload> updateAPWiFiCommand::parsePayload(std::string_view jsonPayload)
{
  UpdateAPWiFiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  // todo implement parsing

  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult updateAPWiFiCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  // todo implement updating
  return CommandResult::getSuccessResult("Config updated");
}
