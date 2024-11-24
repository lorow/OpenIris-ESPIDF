#ifndef COMMAND_RESULT
#define COMMAND_RESULT

#include <format>
#include <string>

class CommandResult
{
private:
  enum class Status
  {
    SUCCESS,
    FAILURE,
  };

  Status status;
  std::string message;

  CommandResult(std::string message, Status status)
  {
    this->status = status;
    if (status == Status::SUCCESS)
    {
      // we gotta do it this way, because if we define it as { "result": " {} " } it crashes the compiler, lol
      this->message = std::format("{}\"result\":\" {} \"{}", "{", message, "}");
    }
    else
    {
      this->message = std::format("{}\"error\":\" {} \"{}", "{", message, "}");
    }
  }

public:
  bool isSuccess() const { return status == Status::SUCCESS; }

  static CommandResult getSuccessResult(std::string message)
  {
    return CommandResult(message, Status::SUCCESS);
  }

  static CommandResult getErrorResult(std::string message)
  {
    return CommandResult(message, Status::FAILURE);
  }

  std::string getResult() const { return this->message; }
};

#endif