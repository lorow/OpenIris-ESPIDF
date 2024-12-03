#include "mdns_commands.hpp"

std::optional<MDNSPayload> setMDNSCommand::parsePayload(std::string_view jsonPayload)
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

CommandResult setMDNSCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
    return CommandResult::getErrorResult("Invalid payload");

  projectConfig->setMDNSConfig(payload.value().hostname, true);

  return CommandResult::getSuccessResult("Config updated");
}
