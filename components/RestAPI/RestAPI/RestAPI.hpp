#pragma once
#ifndef RESTAPI_HPP
#define RESTAPI_HPP
#include <string>
#include <memory>
#include <unordered_map>
#include <mongoose.h>
#include <CommandManager.hpp>

#include "esp_log.h"

#define JSON_RESPONSE "Content-Type: application/json\r\n"

struct RequestContext
{
  mg_connection *connection;
  std::string method;
  std::string body;
};

class RestAPI
{
  using route_handler = void (RestAPI::*)(RequestContext *);
  typedef std::unordered_map<std::string, route_handler> route_map;
  std::string url;
  route_map routes;

  struct mg_mgr mgr;
  std::shared_ptr<CommandManager> command_manager;

private:
  // updates
  void handle_update_wifi(RequestContext *context);
  void handle_update_device(RequestContext *context);
  void handle_update_camera(RequestContext *context);

  // gets
  void handle_get_config(RequestContext *context);

  // resets
  void handle_reset_config(RequestContext *context);

  // reboots
  void handle_reboot(RequestContext *context);
  void handle_camera_reboot(RequestContext *context);

  // heartbeat
  void pong(RequestContext *context);

  // special
  void handle_save(RequestContext *context);

public:
  // this will also need command manager
  RestAPI(std::string url, std::shared_ptr<CommandManager> command_manager);
  void begin();
  void handle_request(struct mg_connection *connection, int event, void *event_data);
  void poll();
};

namespace RestAPIHelpers
{
  void event_handler(struct mg_connection *connection, int event, void *event_data);
};

void HandleRestAPIPollTask(void *pvParameter);

#endif