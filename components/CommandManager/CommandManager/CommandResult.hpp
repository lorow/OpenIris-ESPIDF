#ifndef COMMAND_RESULT
#define COMMAND_RESULT

#include <format>
#include <string>
#include <algorithm>

class CommandResult
{
public:
  enum class Status
  {
    SUCCESS,
    FAILURE,
  };

private:
  Status status;
  std::string message;

public:
  CommandResult(std::string message, const Status status)
  {
    this->status = status;

    // Escape quotes and backslashes in the message for JSON
    std::string escapedMessage = message;
    size_t pos = 0;
    // First escape backslashes
    while ((pos = escapedMessage.find('\\', pos)) != std::string::npos) {
      escapedMessage.replace(pos, 1, "\\\\");
      pos += 2;
    }
    // Then escape quotes
    pos = 0;
    while ((pos = escapedMessage.find('"', pos)) != std::string::npos) {
      escapedMessage.replace(pos, 1, "\\\"");
      pos += 2;
    }

    if (status == Status::SUCCESS)
    {
      this->message = std::format("{{\"result\":\"{}\"}}", escapedMessage);
    }
    else
    {
      this->message = std::format("{{\"error\":\"{}\"}}", escapedMessage);
    }
  }

  bool isSuccess() const { return status == Status::SUCCESS; }

  static CommandResult getSuccessResult(const std::string &message)
  {
    return CommandResult(message, Status::SUCCESS);
  }

  static CommandResult getErrorResult(const std::string &message)
  {
    return CommandResult(message, Status::FAILURE);
  }

  // Create a result that returns raw JSON without wrapper
  static CommandResult getRawJsonResult(const std::string &jsonMessage)
  {
    CommandResult result("", Status::SUCCESS);
    result.message = jsonMessage;
    return result;
  }

  std::string getResult() const { return this->message; }
};

#endif