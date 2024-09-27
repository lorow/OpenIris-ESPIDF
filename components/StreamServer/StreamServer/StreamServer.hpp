#pragma once
#ifndef STREAMSERVER_HPP
#define STREAMSERVER_HPP

#define PART_BOUNDARY "123456789000000000000987654321"

#include "esp_log.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include <StateManager.hpp>
#include <Helpers.hpp>

namespace StreamHelpers
{
  esp_err_t stream(httpd_req_t *req);
}

class StreamServer
{
private:
  httpd_handle_t camera_stream = nullptr;
  int STREAM_SERVER_PORT;

public:
  StreamServer(const int STREAM_PORT = 80);
  esp_err_t startStreamServer();
};

#endif