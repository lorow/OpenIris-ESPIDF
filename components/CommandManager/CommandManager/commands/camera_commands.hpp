#include "BaseCommand.hpp"

class updateCameraCommand : public Command
{
  std::shared_ptr<ProjectConfig> projectConfig;

public:
  updateCameraCommand(std::shared_ptr<ProjectConfig> projectConfig) : projectConfig(projectConfig) {};
  CommandResult execute(std::string_view jsonPayload) override;
  std::optional<UpdateCameraConfigPayload> parsePayload(std::string_view jsonPayload);
};

// add cropping command