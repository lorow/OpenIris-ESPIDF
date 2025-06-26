#pragma once
#ifndef UVCSTREAM_HPP
#define UVCSTREAM_HPP
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_camera.h"
#include <CameraManager.hpp>
#include <StateManager.hpp>
#include "esp_log.h"
#include "usb_device_uvc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// we need access to the camera manager
// in order to update the frame settings
extern std::shared_ptr<CameraManager> cameraHandler;
extern std::shared_ptr<ProjectConfig> deviceConfig;

#ifdef __cplusplus
extern "C" {
#endif

  const char *get_uvc_device_name();
  const char *get_serial_number(void);

#ifdef __cplusplus
}
#endif

// we also need a way to inform the rest of the system of what's happening
extern QueueHandle_t eventQueue;

namespace UVCStreamHelpers
{
  // TODO move the camera handling code to the camera manager and have the uvc manager initialize it in wired mode

  typedef struct
  {
    camera_fb_t *cam_fb_p;
    uvc_fb_t uvc_fb;
  } fb_t;

  static fb_t s_fb;

  static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx);
  static void camera_stop_cb(void *cb_ctx);
  static uvc_fb_t *camera_fb_get_cb(void *cb_ctx);
  static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx);
}

class UVCStreamManager
{
  uint8_t *uvc_buffer = nullptr;

public:
  esp_err_t setup();
};

#endif // UVCSTREAM_HPP
