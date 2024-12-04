#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP

#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "commands/BaseCommand.hpp"
#include "commands/simple_commands.hpp"
#include "commands/camera_commands.hpp"
#include "commands/config_commands.hpp"
#include "commands/mdns_commands.hpp"
#include "commands/wifi_commands.hpp"
#include <cJSON.h>

// mostlikely missing commands
// reset config
// restart device
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
  GET_CONFIG,
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