#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP

#include <ProjectConfig.hpp>
#include <CameraManager.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include <functional>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "DependencyRegistry.hpp"
#include "commands/simple_commands.hpp"
#include "commands/camera_commands.hpp"
#include "commands/config_commands.hpp"
#include "commands/mdns_commands.hpp"
#include "commands/wifi_commands.hpp"
#include "commands/device_commands.hpp"
#include <cJSON.h>

enum class CommandType
{
  None,
  PING,
  SET_WIFI,
  UPDATE_OTA_CREDENTIALS,
  SET_STREAMING_MODE,
  UPDATE_WIFI,
  DELETE_NETWORK,
  UPDATE_AP_WIFI,
  SET_MDNS,
  UPDATE_CAMERA,
  RESTART_CAMERA,
  SAVE_CONFIG,
  GET_CONFIG,
  RESET_CONFIG,
  RESTART_DEVICE,
};

class CommandManager
{
  std::shared_ptr<DependencyRegistry> registry;

public:
  explicit CommandManager(const std::shared_ptr<DependencyRegistry> &DependencyRegistry) : registry(DependencyRegistry) {};
  std::function<CommandResult()> createCommand(CommandType type, std::string_view json) const;

  CommandResult executeFromJson(std::string_view json) const;
  CommandResult executeFromType(CommandType type, std::string_view json) const;
};

#endif