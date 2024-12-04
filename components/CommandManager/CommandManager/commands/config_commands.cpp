#include "config_commands.hpp"

CommandResult saveConfigCommand::execute(std::string_view jsonPayload)
{
  projectConfig->save();
  return CommandResult::getSuccessResult("Config saved");
}

CommandResult getConfigCommand::execute(std::string_view jsonPayload)
{
  auto configRepresentation = projectConfig->getTrackerConfig().toRepresentation();
  return CommandResult::getSuccessResult(configRepresentation);
}