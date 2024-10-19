#include "WebSocketLogger.hpp"

WebSocketLogger::WebSocketLogger()
{
  this->connected_socket_client = async_resp_arg{
      .hd = nullptr,
      .fd = -1,
  };

  this->ws_log_buffer[0] = '\0';
}

void LoggerHelpers::ws_async_send(void *arg)
{
  char *log_buffer = webSocketLogger.get_websocket_log_buffer();

  struct async_resp_arg *resp_arg = (struct async_resp_arg *)arg;
  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;

  httpd_ws_frame_t websocket_packet = httpd_ws_frame_t{};

  websocket_packet.payload = (uint8_t *)log_buffer;
  websocket_packet.len = strlen(log_buffer);
  websocket_packet.type = HTTPD_WS_TYPE_TEXT;

  httpd_ws_send_frame_async(hd, fd, &websocket_packet);
}

esp_err_t WebSocketLogger::log_message(const char *format, va_list args)
{
  vsnprintf(this->ws_log_buffer, 100, format, args);

  if (connected_socket_client.fd == -1 || connected_socket_client.hd == nullptr)
  {
    return ESP_FAIL;
  }

  esp_err_t ret = httpd_queue_work(connected_socket_client.hd, LoggerHelpers::ws_async_send, &connected_socket_client);
  if (ret != ESP_OK)
  {
    connected_socket_client.fd = -1;
    connected_socket_client.hd = nullptr;
  }

  return ret;
}

esp_err_t WebSocketLogger::register_socket_client(httpd_req_t *req)
{
  if (connected_socket_client.fd != -1 && connected_socket_client.hd != nullptr)
  {
    // we're already connected
    return ESP_OK;
  }

  connected_socket_client.hd = req->handle;
  connected_socket_client.fd = httpd_req_to_sockfd(req);
  return ESP_OK;
}

void WebSocketLogger::unregister_socket_client()
{
  connected_socket_client.fd = -1;
  connected_socket_client.hd = nullptr;
}

char *WebSocketLogger::get_websocket_log_buffer()
{
  return this->ws_log_buffer;
}