#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP

#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "Commands.hpp"
#include <cJSON.h>

enum CommandType
{
  None,
  PING,
  SET_WIFI,
  UPDATE_DEVICE,
  UPDATE_WIFI,
  DELETE_NETWORK,
  UPDATE_AP_WIFI,
  UPDATE_MDNS,
  SET_MDNS,
  UPDATE_CAMERA,
  SAVE_CONFIG,
  RESET_CONFIG,
  RESTART_DEVICE,
};

class CommandManager
{
private:
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  CommandManager(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  std::unique_ptr<Command> createCommand(CommandType type);

  CommandResult executeFromJson(std::string_view json);
  CommandResult executeFromType(CommandType type, std::string_view json);
};

#endif