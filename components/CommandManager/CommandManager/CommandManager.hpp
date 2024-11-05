#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"
#include "Commands.hpp"

const enum CommandType {
  None,
  PING,
  SET_WIFI,
  UPDATE_WIFI,
  DELETE_NETWORK,
  SAVE_CONFIG,
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

  CommandResult executeFromJson(const std::string *json);
};