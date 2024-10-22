#pragma once
#ifndef UVCSTREAM_HPP
#define UVCSTREAM_HPP
#include "esp_timer.h"
#include "esp_camera.h"
#include <CameraManager.hpp>
#include "esp_log.h"
#include "usb_device_uvc.h"

// we need access to the camera manager
// in order to update the frame settings
extern CameraManager cameraHandler;

namespace UVCStreamHelpers
{
  // TODO move the camera handling code to the camera manager and have the uvc maanger initialize it in wired mode

  typedef struct
  {
    camera_fb_t *cam_fb_p;
    uvc_fb_t uvc_fb;
  } fb_t;

  static fb_t s_fb;

  static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx);
  static void camera_stop_cb(void *cb_ctx);
  static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx);
  static uvc_fb_t *camera_fb_get_cb(void *cb_ctx);
  static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx);
}

class UVCStreamManager
{
private:
  uint8_t *uvc_buffer;

public:
  esp_err_t setup();
};

#endif // UVCSTREAM_HPP
