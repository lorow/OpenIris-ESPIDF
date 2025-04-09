#include "simple_commands.hpp"

CommandResult PingCommand()
{
  return CommandResult::getSuccessResult("pong");
};
