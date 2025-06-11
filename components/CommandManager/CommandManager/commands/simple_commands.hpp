#ifndef SIMPLE_COMMANDS
#define SIMPLE_COMMANDS

#include <string>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"

CommandResult PingCommand();
CommandResult PauseCommand(const PausePayload& payload);

#endif