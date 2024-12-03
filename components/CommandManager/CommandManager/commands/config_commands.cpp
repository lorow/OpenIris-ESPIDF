#include "config_commands.hpp"

CommandResult saveConfigCommand::execute(std::string_view jsonPayload)
{
  projectConfig->save();
  return CommandResult::getSuccessResult("Config saved");
}