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

std::unordered_map<std::string, CommandType> commandTypeMap = {
    {"ping", CommandType::PING},
    {"set_wifi", CommandType::SET_WIFI},
    {"update_wifi", CommandType::UPDATE_WIFI},
    {"delete_network", CommandType::DELETE_NETWORK},
    {"save_config", CommandType::SAVE_CONFIG},
};

class CommandManager
{
  ProjectConfig &projectConfig;

public:
  CommandManager(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  std::unique_ptr<Command> createCommand(CommandType type);

  CommandResult executeFromJson(std::string *json);
  CommandResult executeFromType(CommandType type, std::string *json);
};

#endif