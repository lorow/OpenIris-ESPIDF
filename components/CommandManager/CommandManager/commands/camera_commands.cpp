#include "camera_commands.hpp"

std::optional<UpdateCameraConfigPayload> parseUpdateCameraPayload(std::string_view jsonPayload)
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

std::optional<RestartCameraPayload> parseRestartCameraPayload(std::string_view jsonPayload)
{
  RestartCameraPayload payload;
  cJSON *parsedJson = cJSON_Parse(jsonPayload.data());

  if (parsedJson == nullptr)
    return std::nullopt;

  cJSON *mode = cJSON_GetObjectItem(parsedJson, "mode");

  if (mode == nullptr)
  {
    cJSON_Delete(parsedJson);
  }

  payload.mode = (bool)mode->valueint;

  cJSON_Delete(parsedJson);

  return payload;
}

CommandResult updateCameraCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
  auto payload = parseUpdateCameraPayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }
  auto updatedConfig = payload.value();

  std::shared_ptr<ProjectConfig> projectConfig = registry->resolve<ProjectConfig>(DependencyType::project_config);
  auto oldConfig = projectConfig->getCameraConfig();
  projectConfig->setCameraConfig(
      updatedConfig.vflip.has_value() ? updatedConfig.vflip.value() : oldConfig.vflip,
      updatedConfig.framesize.has_value() ? updatedConfig.framesize.value() : oldConfig.framesize,
      updatedConfig.href.has_value() ? updatedConfig.href.value() : oldConfig.href,
      updatedConfig.quality.has_value() ? updatedConfig.quality.value() : oldConfig.quality,
      updatedConfig.brightness.has_value() ? updatedConfig.brightness.value() : oldConfig.brightness);

  return CommandResult::getSuccessResult("Config updated");
}

CommandResult restartCameraCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload)
{
  auto payload = parseRestartCameraPayload(jsonPayload);
  if (!payload.has_value())
  {
    return CommandResult::getErrorResult("Invalid payload");
  }

  std::shared_ptr<CameraManager> cameraManager = registry->resolve<CameraManager>(DependencyType::camera_manager);
  cameraManager->resetCamera(payload.value().mode);
  return CommandResult::getSuccessResult("Camera restarted");
}