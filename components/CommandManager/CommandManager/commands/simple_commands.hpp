#ifndef SIMPLE_COMMANDS
#define SIMPLE_COMMANDS

#include <string>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "main_globals.hpp"
#include "esp_log.h"
#include <cJSON.h>

CommandResult PingCommand();
CommandResult PauseCommand(std::string_view jsonPayload);

#endif