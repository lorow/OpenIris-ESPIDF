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

std::optional<ResetConfigPayload> resetConfigCommand::parsePayload(std::string_view jsonPayload)
{
  ResetConfigPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *section = cJSON_GetObjectItem(parsedJson, "section");

  if (section == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.section = std::string(section->valuestring);

  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult resetConfigCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);

  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload or missing section");
  }

  auto sectionPayload = payload.value();

  if (std::find(this->supported_sections.begin(), this->supported_sections.end(), sectionPayload.section) == this->supported_sections.end())
  {
    return CommandResult::getErrorResult("Selected section is unsupported");
  }

  // we cannot match on string, and making a map would be overkill right now, sooo
  // todo, add more granual control for other sections, like only reset camera, or only reset wifi
  if (sectionPayload.section == "all")
    this->projectConfig->reset();

  return CommandResult::getSuccessResult("Config reset");
}