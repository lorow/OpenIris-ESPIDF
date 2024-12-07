#include "BaseCommand.hpp"

class restartDeviceCommand : public Command
{
  CommandResult execute(std::string_view jsonPayload) override;
};