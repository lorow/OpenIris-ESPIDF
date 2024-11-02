#include "RestAPI.hpp"

RestAPI::RestAPI(std::string url)
{
  this->url = url;
  // updates
  routes.emplace("/api/update/wifi/", &RestAPI::handle_wifi);
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
  routes.emplace("/api/reboot/", &RestAPI::handle_reboot);
  routes.emplace("/api/reboot/", &RestAPI::handle_reboot);

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

    bool found = false;
    for (auto it = this->routes.begin(); it != this->routes.end(); it++)
    {
      if (it->first == uri)
      {
        found = true;
        RequestContext *context = new RequestContext{
            .connection = connection,
        };
        auto method = it->second;
        (*this.*method)(context);
      }
    }

    if (!found)
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

void RestAPI::handle_reboot(RequestContext *context)
{
  mg_http_reply(context->connection, 200, "", "Yes sir");
}