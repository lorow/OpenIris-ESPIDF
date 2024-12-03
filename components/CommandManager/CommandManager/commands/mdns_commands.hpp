#include "BaseCommand.hpp"

class setMDNSCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  setMDNSCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<MDNSPayload> parsePayload(std::string_view jsonPayload);
};
