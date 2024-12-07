#ifndef CAMERA_COMMANDS_HPP
#define CAMERA_COMMANDS_HPP
#include "BaseCommand.hpp"
#include <CameraManager.hpp>

class updateCameraCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  updateCameraCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<UpdateCameraConfigPayload> parsePayload(std::string_view jsonPayload);
};

class restartCameraCommand : public Command
{
  std::shared_ptr<CameraManager> cameraManager;

public:
  restartCameraCommand(std::shared_ptr<CameraManager> cameraManager) : cameraManager(cameraManager) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<RestartCameraPayload> parsePayload(std::string_view jsonPayload);
};

#endif
// add cropping command