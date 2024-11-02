#pragma once
#ifndef STREAMSERVER_HPP
#define STREAMSERVER_HPP

#define PART_BOUNDARY "123456789000000000000987654321"

#include "esp_log.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include <StateManager.hpp>
#include <WebSocketLogger.hpp>
#include <Helpers.hpp>

extern WebSocketLogger webSocketLogger;

namespace StreamHelpers
{
  esp_err_t stream(httpd_req_t *req);
  esp_err_t ws_logs_handle(httpd_req_t *req);
}

class StreamServer
{
private:
  int STREAM_SERVER_PORT;
  httpd_handle_t camera_stream = nullptr;

public:
  StreamServer(const int STREAM_PORT);
  esp_err_t startStreamServer();

  esp_err_t stream(httpd_req_t *req);
  esp_err_t ws_logs_handle(httpd_req_t *req);
};

#endif