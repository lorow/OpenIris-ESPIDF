#include "simple_commands.hpp"

CommandResult PingCommand::execute(std::string_view jsonPayload)
{
  return CommandResult::getSuccessResult("pong");
}
