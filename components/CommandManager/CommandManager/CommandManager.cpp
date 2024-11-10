#include "CommandManager.hpp"

std::unique_ptr<Command> CommandManager::createCommand(CommandType type)
{
  switch (type)
  {
  case CommandType::PING:
    return std::make_unique<PingCommand>();
  case CommandType::SET_WIFI:
    return std::make_unique<setWiFiCommand>(projectConfig);
  case CommandType::DELETE_NETWORK:
    return std::make_unique<deleteWifiCommand>(projectConfig);
  default:
    return nullptr;
  }
}

CommandResult CommandManager::executeFromJson(std::string *json)
{
  cJSON *parsedJson = cJSON_Parse(json->c_str());
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

CommandResult CommandManager::executeFromType(CommandType type, std::string *json)
{
  auto command = createCommand(type);

  if (command == nullptr)
  {
    return CommandResult::getErrorResult("Unknown command");
  }

  return command->execute(*json);
}