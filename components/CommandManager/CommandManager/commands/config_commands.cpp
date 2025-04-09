#include "config_commands.hpp"

std::optional<ResetConfigPayload> parseResetConfigCommandPayload(std::string_view jsonPayload)
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

CommandResult saveConfigCommand(std::shared_ptr<DependencyRegistry> registry)
{
  std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);

  projectConfig->save();
  return CommandResult::getSuccessResult("Config saved");
}
CommandResult getConfigCommand(std::shared_ptr<DependencyRegistry> registry)
{
  std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);

  auto configRepresentation = projectConfig->getTrackerConfig().toRepresentation();
  return CommandResult::getSuccessResult(configRepresentation);
}

CommandResult resetConfigCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
  std::array<std::string, 4> supported_sections = {
      "all",
  };

  auto payload = parseResetConfigCommandPayload(jsonPayload);

  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload or missing section");
  }

  auto sectionPayload = payload.value();

  if (std::find(supported_sections.begin(), supported_sections.end(), sectionPayload.section) == supported_sections.end())
  {
    return CommandResult::getErrorResult("Selected section is unsupported");
  }

  // we cannot match on string, and making a map would be overkill right now, sooo
  // todo, add more granual control for other sections, like only reset camera, or only reset wifi
  if (sectionPayload.section == "all")
  {
    std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
    projectConfig->reset();
  }

  return CommandResult::getSuccessResult("Config reset");
}