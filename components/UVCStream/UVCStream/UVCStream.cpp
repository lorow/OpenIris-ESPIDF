#include "UVCStream.hpp"
#include <cstdio> // for snprintf

static const char *UVC_STREAM_TAG = "[UVC DEVICE]";

extern "C" {
  static char serial_number_str[13];

  const char *get_uvc_device_name()
  {
    return deviceConfig->getMDNSConfig().hostname.c_str();
  }

  const char *get_serial_number(void)
  {
    if (serial_number_str[0] == '\0')
    {
      uint8_t mac_address[6];
      esp_err_t result = esp_efuse_mac_get_default(mac_address);
      if (result != ESP_OK)
      {
        ESP_LOGE(UVC_STREAM_TAG, "Failed to get MAC address of the board, returning default serial number");
        return CONFIG_TUSB_SERIAL_NUM;
      }

    // 12 hex chars without separators
    snprintf(serial_number_str, sizeof(serial_number_str), "%02X%02X%02X%02X%02X%02X",
         mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
    }
    return serial_number_str;
  }
}

static esp_err_t UVCStreamHelpers::camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx)
{
  auto *mgr = static_cast<UVCStreamManager *>(cb_ctx);
  ESP_LOGI(UVC_STREAM_TAG, "Camera Start");
  ESP_LOGI(UVC_STREAM_TAG, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);
  framesize_t frame_size = FRAMESIZE_QVGA;

  if (format != UVC_FORMAT_JPEG)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Only support MJPEG format");
    return ESP_ERR_NOT_SUPPORTED;
  }

  if (width == 240 && height == 240)
  {
    frame_size = FRAMESIZE_240X240;
  }
  else
  {
    ESP_LOGE(UVC_STREAM_TAG, "Unsupported frame size %dx%d", width, height);
    return ESP_ERR_NOT_SUPPORTED;
  }

  cameraHandler->setCameraResolution(frame_size);
  cameraHandler->resetCamera(false);

  constexpr SystemEvent event = {EventSource::STREAM, StreamState_e::Stream_ON};
  xQueueSend(eventQueue, &event, 10);

  return ESP_OK;
}

static void UVCStreamHelpers::camera_stop_cb(void *cb_ctx)
{
  (void)cb_ctx;
  if (s_fb.cam_fb_p)
  {
    esp_camera_fb_return(s_fb.cam_fb_p);
    s_fb.cam_fb_p = nullptr;
  }

  constexpr SystemEvent event = {EventSource::STREAM, StreamState_e::Stream_OFF};
  xQueueSend(eventQueue, &event, 10);
}

static uvc_fb_t *UVCStreamHelpers::camera_fb_get_cb(void *cb_ctx)
{
  auto *mgr = static_cast<UVCStreamManager *>(cb_ctx);
  s_fb.cam_fb_p = esp_camera_fb_get();

  if (!s_fb.cam_fb_p)
  {
    return nullptr;
  }

  s_fb.uvc_fb.buf = s_fb.cam_fb_p->buf;
  s_fb.uvc_fb.len = s_fb.cam_fb_p->len;
  s_fb.uvc_fb.width = s_fb.cam_fb_p->width;
  s_fb.uvc_fb.height = s_fb.cam_fb_p->height;
  s_fb.uvc_fb.format = UVC_FORMAT_JPEG; // we gotta make sure we're ALWAYS using JPEG
  s_fb.uvc_fb.timestamp = s_fb.cam_fb_p->timestamp;

  // Ensure frame fits into configured UVC transfer buffer
  if (mgr && s_fb.uvc_fb.len > mgr->getUvcBufferSize())
  {
    ESP_LOGE(UVC_STREAM_TAG, "Frame size %d exceeds UVC buffer size %u", (int)s_fb.uvc_fb.len, (unsigned)mgr->getUvcBufferSize());
    esp_camera_fb_return(s_fb.cam_fb_p);
    return nullptr;
  }

  return &s_fb.uvc_fb;
}

static void UVCStreamHelpers::camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx)
{
  (void)cb_ctx;
  assert(fb == &s_fb.uvc_fb);
  esp_camera_fb_return(s_fb.cam_fb_p);
}

esp_err_t UVCStreamManager::setup()
{

#ifndef CONFIG_GENERAL_WIRED_MODE
  ESP_LOGE(UVC_STREAM_TAG, "The board does not support UVC, please, setup WiFi connection.");
  return ESP_FAIL;
#endif

  ESP_LOGI(UVC_STREAM_TAG, "Setting up UVC Stream");

  // Derive transfer buffer size from configuration/descriptor settings.
  // Use a conservative default if not defined elsewhere.
  // For now, allocate based on commonly used max frame size or future Kconfig value.
  // You can wire this to a Kconfig like CONFIG_UVC_MAX_FRAME_SIZE.
  if (uvc_buffer_size == 0)
  {
    // default to 75 KiB if not set elsewhere
    uvc_buffer_size = 75 * 1024;
  }
  uvc_buffer = static_cast<uint8_t *>(malloc(uvc_buffer_size));
  if (uvc_buffer == nullptr)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Allocating buffer for UVC Device failed");
    return ESP_FAIL;
  }

  uvc_device_config_t config = {
      .uvc_buffer = uvc_buffer,
      .uvc_buffer_size = uvc_buffer_size,
      .start_cb = UVCStreamHelpers::camera_start_cb,
      .fb_get_cb = UVCStreamHelpers::camera_fb_get_cb,
      .fb_return_cb = UVCStreamHelpers::camera_fb_return_cb,
      .stop_cb = UVCStreamHelpers::camera_stop_cb,
      .cb_ctx = this,
  };

  esp_err_t ret = uvc_device_config(0, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Configuring UVC Device failed: %s", esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(UVC_STREAM_TAG, "Configured UVC Device");

  ESP_LOGI(UVC_STREAM_TAG, "Initializing UVC Device");
  ret = uvc_device_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Initializing UVC Device failed: %s", esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(UVC_STREAM_TAG, "Initialized UVC Device");

  return ESP_OK;
}

esp_err_t UVCStreamManager::start()
{
  ESP_LOGI(UVC_STREAM_TAG, "Starting UVC streaming");
  // UVC device is already initialized in setup(), just log that we're starting
  return ESP_OK;
}