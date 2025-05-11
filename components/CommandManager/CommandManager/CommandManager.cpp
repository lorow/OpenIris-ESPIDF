#include "CommandManager.hpp"

std::unordered_map<std::string, CommandType> commandTypeMap = {
    {"ping", CommandType::PING},
    {"set_wifi", CommandType::SET_WIFI},
    {"update_wifi", CommandType::UPDATE_WIFI},
    {"update_device", CommandType::UPDATE_DEVICE},
    {"delete_network", CommandType::DELETE_NETWORK},
    {"update_ap_wifi", CommandType::UPDATE_AP_WIFI},
    {"update_mdns", CommandType::UPDATE_MDNS},
    {"set_mdns", CommandType::SET_MDNS},
    {"update_camera", CommandType::UPDATE_CAMERA},
    {"restart_camera", CommandType::RESTART_CAMERA},
    {"save_config", CommandType::SAVE_CONFIG},
    {"get_config", CommandType::GET_CONFIG},
    {"reset_config", CommandType::RESET_CONFIG},
    {"restart_device", CommandType::RESTART_DEVICE},
};

std::function<CommandResult()> CommandManager::createCommand(CommandType type, std::string_view json)
{
  typedef std::function<CommandResult()> CommandFunction;

  switch (type)
  {
  case CommandType::PING:
    return CommandFunction(PingCommand);
  case CommandType::SET_WIFI:
    return CommandFunction(std::bind(setWiFiCommand, this->registry, json));
  case CommandType::UPDATE_WIFI:
    return CommandFunction(std::bind(updateWiFiCommand, this->registry, json));
  case CommandType::UPDATE_AP_WIFI:
    return CommandFunction(std::bind(updateAPWiFiCommand, this->registry, json));
  case CommandType::DELETE_NETWORK:
    return CommandFunction(std::bind(deleteWiFiCommand, this->registry, json));
  case CommandType::SET_MDNS:
    return CommandFunction(std::bind(setMDNSCommand, this->registry, json));
  // updating the mnds name is essentially the same operation
  case CommandType::UPDATE_MDNS:
    return CommandFunction(std::bind(setMDNSCommand, this->registry, json));
  case CommandType::UPDATE_CAMERA:
    return CommandFunction(std::bind(updateCameraCommand, this->registry, json));
  case CommandType::RESTART_CAMERA:
    return CommandFunction(std::bind(restartCameraCommand, this->registry, json));
  case CommandType::GET_CONFIG:
    return CommandFunction(std::bind(getConfigCommand, this->registry));
  case CommandType::SAVE_CONFIG:
    return CommandFunction(std::bind(saveConfigCommand, this->registry));
  case CommandType::RESET_CONFIG:
    return CommandFunction(std::bind(resetConfigCommand, this->registry, json));
  case CommandType::RESTART_DEVICE:
    return CommandFunction(restartDeviceCommand);
  default:
    return nullptr;
  }
}

CommandResult CommandManager::executeFromJson(std::string_view json)
{
  std::string results = "";

  cJSON *parsedJson = cJSON_Parse(json.data());
  if (parsedJson == nullptr)
    return CommandResult::getErrorResult("Initial JSON Parse: Invalid JSON");

  const cJSON *commandData = nullptr;
  const cJSON *commands = cJSON_GetObjectItem(parsedJson, "commands");

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
    auto commandType = commandTypeMap.at(std::string(commandTypeString->valuestring));

    std::string commandPayloadString = "";
    if (commandPayload != nullptr)
      commandPayloadString = std::string(cJSON_Print(commandPayload));

    auto command = createCommand(commandType, commandPayloadString);
    if (command == nullptr)
    {
      cJSON_Delete(parsedJson);
      return CommandResult::getErrorResult("Unknown command");
    }

    // todo, make it a proper json later
    results += command().getResult() + ", \n";
  }

  cJSON_Delete(parsedJson);
  return CommandResult::getSuccessResult(results);
}

CommandResult CommandManager::executeFromType(CommandType type, std::string_view json)
{
  auto command = createCommand(type, json);

  if (command == nullptr)
  {
    return CommandResult::getErrorResult("Unknown command");
  }

  return command();
}