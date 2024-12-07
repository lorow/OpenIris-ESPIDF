#include "BaseCommand.hpp"

class saveConfigCommand : public Command
{
public:
  std::shared_ptr<ProjectConfig> projectConfig;
  saveConfigCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
};

class getConfigCommand : public Command
{
public:
  std::shared_ptr<ProjectConfig> projectConfig;
  getConfigCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
};

class resetConfigCommand : public Command
{
  std::array<std::string, 4> supported_sections = {
      "all",
  };

public:
  std::shared_ptr<ProjectConfig> projectConfig;
  resetConfigCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<ResetConfigPayload> parsePayload(std::string_view jsonPayload);
};