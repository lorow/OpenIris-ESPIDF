#include "RestAPI.hpp"

#define POST_METHOD "POST"

RestAPI::RestAPI(std::string url, std::shared_ptr<CommandManager> commandManager) : command_manager(commandManager)
{
  this->url = url;
  // updates
  routes.emplace("/api/update/wifi/", &RestAPI::handle_update_wifi);
  routes.emplace("/api/update/device/", &RestAPI::handle_update_device);
  routes.emplace("/api/update/camera/", &RestAPI::handle_update_camera);

  // post will reset it
  // resets
  routes.emplace("/api/reset/config/", &RestAPI::handle_reset_config);
  // gets
  routes.emplace("/api/get/config/", &RestAPI::handle_get_config);

  // reboots
  routes.emplace("/api/reboot/device/", &RestAPI::handle_reboot);
  routes.emplace("/api/reboot/camera/", &RestAPI::handle_camera_reboot);

  // heartbeat
  routes.emplace("/api/ping/", &RestAPI::pong);

  // special
  routes.emplace("/api/save/", &RestAPI::handle_save);
}

void RestAPI::begin()
{
  mg_log_set(MG_LL_DEBUG);
  mg_mgr_init(&mgr);
  // every route is handled through this class, with commands themselves by a command manager
  // hence we pass a pointer to this in mg_http_listen
  mg_http_listen(&mgr, this->url.c_str(), (mg_event_handler_t)RestAPIHelpers::event_handler, this);
}

void RestAPI::handle_request(struct mg_connection *connection, int event, void *event_data)
{
  if (event == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *message = (struct mg_http_message *)event_data;
    std::string uri = std::string(message->uri.buf, message->uri.len);
    auto handler = this->routes[uri];

    if (handler)
    {
      RequestContext *context = new RequestContext{
          .connection = connection,
          .method = std::string(message->method.buf, message->method.len),
          .body = std::string(message->body.buf, message->body.len),
      };
      (*this.*handler)(context);
    }
    else
    {
      mg_http_reply(connection, 404, "", "Wrong URL");
    }
  }
}

void RestAPIHelpers::event_handler(struct mg_connection *connection, int event, void *event_data)
{
  RestAPI *rest_api_handler = static_cast<RestAPI *>(connection->fn_data);
  rest_api_handler->handle_request(connection, event, event_data);
}

void RestAPI::poll()
{
  mg_mgr_poll(&mgr, 100);
}

// COMMANDS
// updates
void RestAPI::handle_update_wifi(RequestContext *context)
{
  if (context->method != POST_METHOD)
  {
    mg_http_reply(context->connection, 401, JSON_RESPONSE, "{%m:%m}", MG_ESC("error"), "Method not allowed");
    return;
  }

  auto result = command_manager->executeFromType(CommandType::UPDATE_WIFI, context->body);
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, result.getResult().c_str());
}

void RestAPI::handle_update_device(RequestContext *context)
{
  if (context->method != POST_METHOD)
  {
    mg_http_reply(context->connection, 401, JSON_RESPONSE, "{%m:%m}", MG_ESC("error"), "Method not allowed");
    return;
  }

  auto result = command_manager->executeFromType(CommandType::UPDATE_DEVICE, context->body);
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, result.getResult().c_str());
}

void RestAPI::handle_update_camera(RequestContext *context)
{
  if (context->method != POST_METHOD)
  {
    mg_http_reply(context->connection, 401, JSON_RESPONSE, "{%m:%m}", MG_ESC("error"), "Method not allowed");
    return;
  }

  auto result = command_manager->executeFromType(CommandType::UPDATE_CAMERA, context->body);
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, result.getResult().c_str());
}

// gets

void RestAPI::handle_get_config(RequestContext *context)
{
  auto result = this->command_manager->executeFromType(CommandType::GET_CONFIG, "");
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), result.getResult());
}

// resets

void RestAPI::handle_reset_config(RequestContext *context)
{
  if (context->method != POST_METHOD)
  {
    mg_http_reply(context->connection, 401, JSON_RESPONSE, "{%m:%m}", MG_ESC("error"), "Method not allowed");
    return;
  }

  auto result = this->command_manager->executeFromType(CommandType::RESET_CONFIG, "{\"section\": \"all\"}");
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), result.getResult());
}

// reboots
void RestAPI::handle_reboot(RequestContext *context)
{
  auto result = this->command_manager->executeFromType(CommandType::RESTART_DEVICE, "");
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Ok");
}

void RestAPI::handle_camera_reboot(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Ok");
}

// heartbeat

void RestAPI::pong(RequestContext *context)
{
  CommandResult result = this->command_manager->executeFromType(CommandType::PING, "");
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, result.getResult().c_str());
}

// special

void RestAPI::handle_save(RequestContext *context)
{
  CommandResult result = this->command_manager->executeFromType(CommandType::SAVE_CONFIG, "");
  int code = result.isSuccess() ? 200 : 500;
  mg_http_reply(context->connection, code, JSON_RESPONSE, result.getResult().c_str());
}