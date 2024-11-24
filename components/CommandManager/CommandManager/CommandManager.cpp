#include "CommandManager.hpp"

std::unordered_map<std::string, CommandType> commandTypeMap = {
    {"ping", CommandType::PING},
    {"set_wifi", CommandType::SET_WIFI},
    {"update_wifi", CommandType::UPDATE_WIFI},
    {"delete_network", CommandType::DELETE_NETWORK},
    {"save_config", CommandType::SAVE_CONFIG},
};

std::unique_ptr<Command> CommandManager::createCommand(CommandType type)
{
  switch (type)
  {
  case CommandType::PING:
    return std::make_unique<PingCommand>();
  case CommandType::SET_WIFI:
    return std::make_unique<setWiFiCommand>(projectConfig);
  case CommandType::UPDATE_WIFI:
    return std::make_unique<updateWifiCommand>(projectConfig);
  case CommandType::UPDATE_AP_WIFI:
    return std::make_unique<updateAPWiFiCommand>(projectConfig);
  case CommandType::DELETE_NETWORK:
    return std::make_unique<deleteWifiCommand>(projectConfig);
  case CommandType::SET_MDNS:
    return std::make_unique<setMDNSCommand>(projectConfig);
  // updating the mnds name is essentially the same operation
  case CommandType::UPDATE_MDNS:
    return std::make_unique<setMDNSCommand>(projectConfig);
  case CommandType::UPDATE_CAMERA:
    return std::make_unique<updateCameraCommand>(projectConfig);
  case CommandType::SAVE_CONFIG:
    return std::make_unique<saveConfigCommand>(projectConfig);
  default:
    return nullptr;
  }
}

CommandResult CommandManager::executeFromJson(std::string_view json)
{
  cJSON *parsedJson = cJSON_Parse(json.data());
  if (parsedJson == nullptr)
    return CommandResult::getErrorResult("Invalid JSON");

  const cJSON *commandData = nullptr;
  const cJSON *commands = cJSON_GetObjectItem(parsedJson, "commands");
  cJSON_ArrayForEach(commandData, commands)
  {
    const cJSON *commandTypeString = cJSON_GetObjectItem(commandData, "command");
    const cJSON *commandPayload = cJSON_GetObjectItem(commandData, "data");

    auto commandType = commandTypeMap.at(std::string(commandTypeString->valuestring));
    auto command = createCommand(commandType);

    if (command == nullptr)
    {
      cJSON_Delete(parsedJson);
      return CommandResult::getErrorResult("Unknown command");
    }

    std::string commandPayloadString = std::string(cJSON_Print(commandPayload));
    cJSON_Delete(parsedJson);
    return command->execute(commandPayloadString);
  }

  cJSON_Delete(parsedJson);
  return CommandResult::getErrorResult("Commands missing");
}

CommandResult CommandManager::executeFromType(CommandType type, std::string_view json)
{
  auto command = createCommand(type);

  if (command == nullptr)
  {
    return CommandResult::getErrorResult("Unknown command");
  }

  return command->execute(json);
}