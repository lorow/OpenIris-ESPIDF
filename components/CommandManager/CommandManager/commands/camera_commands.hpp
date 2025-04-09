#ifndef CAMERA_COMMANDS_HPP
#define CAMERA_COMMANDS_HPP
#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <cJSON.h>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "DependencyRegistry.hpp"
#include <CameraManager.hpp>

std::optional<UpdateCameraConfigPayload> parseUpdateCameraPayload(std::string_view jsonPayload);
CommandResult updateCameraCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);

std::optional<RestartCameraPayload> parseRestartCameraPayload(std::string_view jsonPayload);
CommandResult restartCameraCommand(std::shared_ptr<DependencyRegistry> registry, std::string_view jsonPayload);
#endif

// add cropping command