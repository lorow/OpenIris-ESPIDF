#include "CommandManager.hpp"

std::unique_ptr<Command> CommandManager::createCommand(CommandType type)
{
  switch (type)
  {
  case CommandType::PING:
    return std::make_unique<PingCommand>();
    break;
  case CommandType::UPDATE_WIFI:
    return std::make_unique<UpdateWifiCommand>(projectConfig);
  default:
    return nullptr;
  }
}

CommandResult CommandManager::executeFromJson(const std::string *json)
{
  // parse it, get a type of command, grab a command from the map based on type
  // parse the payload json, call execute on the command and return the result
}