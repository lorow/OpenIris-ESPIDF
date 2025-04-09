#include "mdns_commands.hpp"

std::optional<MDNSPayload> parseMDNSCommandPayload(std::string_view jsonPayload)
{
  MDNSPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());
  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *hostnameObject = cJSON_GetObjectItem(parsedJson, "hostname");

  if (hostnameObject == nullptr)
  {
    cJSON_Delete(parsedJson);
    return std::nullopt;
  }

  payload.hostname = std::string(hostnameObject->valuestring);
  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult setMDNSCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
  auto payload = parseMDNSCommandPayload(jsonPayload);
  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
  projectConfig->setMDNSConfig(payload.value().hostname);

  return CommandResult::getSuccessResult("Config updated");
}
