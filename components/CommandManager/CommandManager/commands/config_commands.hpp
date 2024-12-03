#include "BaseCommand.hpp"

class saveConfigCommand : public Command
{
public:
  std::shared_ptr<ProjectConfig> projectConfig;
  saveConfigCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
};