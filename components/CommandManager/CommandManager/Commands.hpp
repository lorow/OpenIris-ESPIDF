#include <ProjectConfig.hpp>
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include "CommandResult.hpp"
#include "CommandSchema.hpp"

class Command
{
  virtual ~Command() = default;
  virtual CommandResult execute(const std::shared_ptr<BasePayload> &payload) = 0;
  virtual std::shared_ptr<BasePayload> parsePayload(const std::string *json) = 0;
};

class PingCommand : public Command
{
public:
  CommandResult execute(const std::shared_ptr<BasePayload> &payload);
  std::shared_ptr<BasePayload> parsePayload(const std::string *json);
};

class setConfigCommand : public Command
{
public:
  CommandResult execute(const std::shared_ptr<BasePayload> &payload);
  std::shared_ptr<BasePayload> parsePayload(const std::string *json);
};

class UpdateWifiCommand : public Command
{
private:
  ProjectConfig &projectConfig;

public:
  UpdateWifiCommand(ProjectConfig &projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(const std::shared_ptr<BasePayload> &payload);
};
