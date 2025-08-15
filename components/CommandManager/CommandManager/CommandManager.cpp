#include "CommandManager.hpp"
#include <cstdlib>

std::unordered_map<std::string, CommandType> commandTypeMap = {
    {"ping", CommandType::PING},
    {"pause", CommandType::PAUSE},
    {"set_wifi", CommandType::SET_WIFI},
    {"update_wifi", CommandType::UPDATE_WIFI},
    {"set_streaming_mode", CommandType::SET_STREAMING_MODE},
    {"update_ota_credentials", CommandType::UPDATE_OTA_CREDENTIALS},
    {"delete_network", CommandType::DELETE_NETWORK},
    {"update_ap_wifi", CommandType::UPDATE_AP_WIFI},
    {"set_mdns", CommandType::SET_MDNS},
    {"update_camera", CommandType::UPDATE_CAMERA},
    {"restart_camera", CommandType::RESTART_CAMERA},
    {"save_config", CommandType::SAVE_CONFIG},
    {"get_config", CommandType::GET_CONFIG},
    {"reset_config", CommandType::RESET_CONFIG},
    {"restart_device", CommandType::RESTART_DEVICE},
    {"scan_networks", CommandType::SCAN_NETWORKS},
    {"start_streaming", CommandType::START_STREAMING},
    {"get_wifi_status", CommandType::GET_WIFI_STATUS},
    {"connect_wifi", CommandType::CONNECT_WIFI},
    {"switch_mode", CommandType::SWITCH_MODE},
    {"get_device_mode", CommandType::GET_DEVICE_MODE},
};

std::function<CommandResult()> CommandManager::createCommand(const CommandType type, std::string_view json) const {
  switch (type)
  {
  case CommandType::PING:
    return { PingCommand };
  case CommandType::PAUSE:
    return [json] { return PauseCommand(json); };
  case CommandType::SET_STREAMING_MODE:
      return [this, json] {return setDeviceModeCommand(this->registry, json); };
  case CommandType::UPDATE_OTA_CREDENTIALS:
      return [this, json] { return updateOTACredentialsCommand(this->registry, json); };
  case CommandType::SET_WIFI:
    return [this, json] { return setWiFiCommand(this->registry, json); };
  case CommandType::UPDATE_WIFI:
    return [this, json] { return updateWiFiCommand(this->registry, json); };
  case CommandType::UPDATE_AP_WIFI:
    return [this, json] { return updateAPWiFiCommand(this->registry, json); };
  case CommandType::DELETE_NETWORK:
    return [this, json] { return deleteWiFiCommand(this->registry, json); };
  case CommandType::SET_MDNS:
    return [this, json] { return setMDNSCommand(this->registry, json); };
  case CommandType::UPDATE_CAMERA:
      return [this, json] { return updateCameraCommand(this->registry, json); };
  case CommandType::RESTART_CAMERA:
    return [this, json] { return restartCameraCommand(this->registry, json); };
  case CommandType::GET_CONFIG:
    return [this] { return getConfigCommand(this->registry); };
  case CommandType::SAVE_CONFIG:
    return [this] { return saveConfigCommand(this->registry); };
  case CommandType::RESET_CONFIG:
    return [this, json] { return resetConfigCommand(this->registry, json); };
  case CommandType::RESTART_DEVICE:
    return restartDeviceCommand;
  case CommandType::SCAN_NETWORKS:
    return [this] { return scanNetworksCommand(this->registry); };
  case CommandType::START_STREAMING:
    return startStreamingCommand;
  case CommandType::GET_WIFI_STATUS:
    return [this] { return getWiFiStatusCommand(this->registry); };
  case CommandType::CONNECT_WIFI:
    return [this] { return connectWiFiCommand(this->registry); };
  case CommandType::SWITCH_MODE:
    return [this, json] { return switchModeCommand(this->registry, json); };
  case CommandType::GET_DEVICE_MODE:
    return [this] { return getDeviceModeCommand(this->registry); };
  default:
    return nullptr;
  }
}

CommandResult CommandManager::executeFromJson(const std::string_view json) const
{
  cJSON *parsedJson = cJSON_Parse(json.data());
  if (parsedJson == nullptr)
    return CommandResult::getErrorResult("Initial JSON Parse: Invalid JSON");

  const cJSON *commandData = nullptr;
  const cJSON *commands = cJSON_GetObjectItem(parsedJson, "commands");
  cJSON *responseDocument = cJSON_CreateObject();
  cJSON *responses = cJSON_CreateArray();
  cJSON *response = nullptr;

  cJSON_AddItemToObject(responseDocument, "results", responses);

  if (cJSON_GetArraySize(commands) == 0)
  {
    cJSON_Delete(parsedJson);
    return CommandResult::getErrorResult("Commands missing");
  }

  cJSON_ArrayForEach(commandData, commands)
  {
    const cJSON *commandTypeString = cJSON_GetObjectItem(commandData, "command");
    if (commandTypeString == nullptr)
    {
      return CommandResult::getErrorResult("Unknown command - missing command type");
    }

    const cJSON *commandPayload = cJSON_GetObjectItem(commandData, "data");
    const auto commandType = commandTypeMap.at(std::string(commandTypeString->valuestring));

    std::string commandPayloadString;
    if (commandPayload != nullptr)
      commandPayloadString = std::string(cJSON_Print(commandPayload));

    auto command = createCommand(commandType, commandPayloadString);
    if (command == nullptr)
    {
      cJSON_Delete(parsedJson);
      return CommandResult::getErrorResult("Unknown command");
    }
    response = cJSON_CreateString(command().getResult().c_str());
    cJSON_AddItemToArray(responses, response);
  }

  char *jsonString = cJSON_Print(responseDocument);
  cJSON_Delete(responseDocument);
  cJSON_Delete(parsedJson);

  // Return the JSON response directly without wrapping it
  // The responseDocument already contains the proper format: {"results": [...]}
  CommandResult result = CommandResult::getRawJsonResult(jsonString);
  free(jsonString);
  return result;
}

CommandResult CommandManager::executeFromType(const CommandType type, const std::string_view json) const
{
  const auto command = createCommand(type, json);

  if (command == nullptr)
  {
    return CommandResult::getErrorResult("Unknown command");
  }

  return command();
}