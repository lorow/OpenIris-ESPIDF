#include "Commands.hpp"

CommandResult PingCommand::execute(std::string &jsonPayload)
{
  return CommandResult::getSuccessResult("pong");
}

std::optional<WifiPayload> setWiFiCommand::parsePayload(std::string &jsonPayload)
{
  WifiPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.c_str());

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

CommandResult setWiFiCommand::execute(std::string &jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }
  auto wifiConfig = payload.value();
  projectConfig.setWifiConfig(
      wifiConfig.networkName,
      wifiConfig.ssid,
      wifiConfig.password,
      wifiConfig.channel,
      wifiConfig.power,
      false,
      false);

  return CommandResult::getSuccessResult("Config updated");
}

std::optional<deleteNetworkPayload> deleteWifiCommand::parsePayload(std::string &jsonPayload)
{
  deleteNetworkPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.c_str());

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

CommandResult deleteWifiCommand::execute(std::string &jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  projectConfig.deleteWifiConfig(payload.value().networkName, false);
  return CommandResult::getSuccessResult("Config updated");
}