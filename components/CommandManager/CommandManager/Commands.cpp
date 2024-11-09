#include "Commands.hpp"

CommandResult PingCommand::execute(std::string &jsonPayload)
{
  return CommandResult::getSuccessResult("pong");
}

WifiPayload setConfigCommand::parsePayload(std::string &jsonPayload)
{
  return WifiPayload();
}

CommandResult setConfigCommand::execute(std::string &jsonPayload)
{
  return CommandResult::getSuccessResult("pong");
}
