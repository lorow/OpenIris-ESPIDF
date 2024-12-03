#include "camera_commands.hpp"

std::optional<UpdateCameraConfigPayload> updateCameraCommand::parsePayload(std::string_view jsonPayload)
{
  UpdateCameraConfigPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *vflipObject = cJSON_GetObjectItem(parsedJson, "vflip");
  cJSON *hrefObject = cJSON_GetObjectItem(parsedJson, "href");
  cJSON *framesize = cJSON_GetObjectItem(parsedJson, "framesize");
  cJSON *quality = cJSON_GetObjectItem(parsedJson, "quality");
  cJSON *brightness = cJSON_GetObjectItem(parsedJson, "brightness");

  if (vflipObject != nullptr)
    payload.vflip = vflipObject->valueint;
  if (hrefObject != nullptr)
    payload.href = hrefObject->valueint;
  if (framesize != nullptr)
    payload.framesize = framesize->valueint;
  if (quality != nullptr)
    payload.quality = quality->valueint;
  if (brightness != nullptr)
    payload.brightness = brightness->valueint;

  cJSON_Delete(parsedJson);
  return payload;
}

CommandResult updateCameraCommand::execute(std::string_view jsonPayload)
{
  auto payload = parsePayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }
  auto updatedConfig = payload.value();

  auto oldConfig = projectConfig->getCameraConfig();
  this->projectConfig->setCameraConfig(
      updatedConfig.vflip.has_value() ? updatedConfig.vflip.value() : oldConfig.vflip,
      updatedConfig.framesize.has_value() ? updatedConfig.framesize.value() : oldConfig.framesize,
      updatedConfig.href.has_value() ? updatedConfig.href.value() : oldConfig.href,
      updatedConfig.quality.has_value() ? updatedConfig.quality.value() : oldConfig.quality,
      updatedConfig.brightness.has_value() ? updatedConfig.brightness.value() : oldConfig.brightness,
      true);

  return CommandResult::getSuccessResult("Config updated");
}
