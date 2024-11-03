#include "RestAPI.hpp"

RestAPI::RestAPI(std::string url)
{
  this->url = url;
  // updates
  routes.emplace("/api/update/wifi/", &RestAPI::handle_update_wifi);
  routes.emplace("/api/update/device/", &RestAPI::handle_update_device);
  routes.emplace("/api/update/camera/", &RestAPI::handle_update_camera);

  // post will reset it
  // resets
  routes.emplace("/api/reset/config/", &RestAPI::handle_reset_config);
  routes.emplace("/api/reset/wifi/", &RestAPI::handle_reset_wifi_config);
  routes.emplace("/api/reset/txpower/", &RestAPI::handle_reset_txpower_config);
  routes.emplace("/api/reset/camera/", &RestAPI::handle_reset_camera_config);

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

    if (auto method_it = this->routes.find(uri); method_it != this->routes.end())
    {
      RequestContext *context = new RequestContext{
          .connection = connection,
          .method = std::string(message->method.buf, message->method.len),
          .body = std::string(message->body.buf, message->body.len),
      };
      auto method = method_it->second;
      (*this.*method)(context);
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

// commands

// updates
void RestAPI::handle_update_wifi(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "WiFi config updated");
}

void RestAPI::handle_update_device(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Device config updated");
}

void RestAPI::handle_update_camera(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Device config updated");
}

// gets

void handle_get_config(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Device config updated");
}

// resets

void RestAPI::handle_reset_config(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Config reset");
}

void RestAPI::handle_reset_wifi_config(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "WiFi Config reset");
}

void RestAPI::handle_reset_txpower_config(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "TX Power Config reset");
}

void RestAPI::handle_reset_camera_config(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Camera Config reset");
}

// reboots
void RestAPI::handle_reboot(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Ok");
}

// heartbeat

void RestAPI::pong(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "pong");
}

// special

void RestAPI::handle_save(RequestContext *context)
{
  mg_http_reply(context->connection, 200, JSON_RESPONSE, "{%m:%m}", MG_ESC("result"), "Config saved");
}