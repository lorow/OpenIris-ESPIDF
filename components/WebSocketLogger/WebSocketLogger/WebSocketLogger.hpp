#pragma once
#ifndef WEBSOCKETLOGGER_HPP
#define WEBSOCKETLOGGER_HPP

#include "esp_http_server.h"
#include "esp_log.h"

#define WS_LOG_BUFFER_LEN 1024

struct async_resp_arg
{
  httpd_handle_t hd;
  int fd;
};

namespace LoggerHelpers
{
  void ws_async_send(void *arg);
}

class WebSocketLogger
{
private:
  async_resp_arg connected_socket_client;
  char ws_log_buffer[WS_LOG_BUFFER_LEN];

public:
  WebSocketLogger();

  esp_err_t log_message(const char *format, va_list args);
  esp_err_t register_socket_client(httpd_req_t *req);
  void unregister_socket_client();
  bool is_client_connected();
  char *get_websocket_log_buffer();
};

extern WebSocketLogger webSocketLogger;

#endif