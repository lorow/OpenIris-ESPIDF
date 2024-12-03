#include "BaseCommand.hpp"

class PingCommand : public Command
{
public:
  PingCommand() = default;
  CommandResult execute(std::string_view jsonPayload) override;
};
