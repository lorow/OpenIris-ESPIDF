#include "UVCStream.hpp"
constexpr int UVC_MAX_FRAMESIZE_SIZE(75 * 1024);

static const char *UVC_STREAM_TAG = "[UVC DEVICE]";

// debug only
static esp_err_t camera_init(int xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, int jpeg_quality, uint8_t fb_count)
{
  static bool inited = false;
  static int cur_xclk_freq_hz = 20000000;
  static pixformat_t cur_pixel_format = PIXFORMAT_JPEG;
  static framesize_t cur_frame_size = FRAMESIZE_96X96;
  static int cur_jpeg_quality = 0;
  static uint8_t cur_fb_count = 0;

  if ((inited && cur_xclk_freq_hz == xclk_freq_hz && cur_pixel_format == pixel_format && cur_frame_size == frame_size && cur_fb_count == fb_count && cur_jpeg_quality == jpeg_quality))
  {
    ESP_LOGD(UVC_STREAM_TAG, "camera already inited");
    return ESP_OK;
  }
  else if (inited)
  {
    esp_camera_return_all();
    esp_camera_deinit();
    inited = false;
    ESP_LOGI(UVC_STREAM_TAG, "camera RESTART");
  }

  camera_config_t camera_config = {
      .pin_pwdn = -1,     // CAM_PIN_PWDN,
      .pin_reset = -1,    // CAM_PIN_RESET,
      .pin_xclk = 10,     // CAM_PIN_XCLK,
      .pin_sccb_sda = 40, // CAM_PIN_SIOD,
      .pin_sccb_scl = 39, // CAM_PIN_SIOC,

      .pin_d7 = 48,    /// CAM_PIN_D7,
      .pin_d6 = 11,    /// CAM_PIN_D6,
      .pin_d5 = 12,    // CAM_PIN_D5,
      .pin_d4 = 14,    // CAM_PIN_D4,
      .pin_d3 = 16,    // CAM_PIN_D3,
      .pin_d2 = 18,    // CAM_PIN_D2,
      .pin_d1 = 17,    // CAM_PIN_D1,
      .pin_d0 = 15,    // CAM_PIN_D0,
      .pin_vsync = 38, // CAM_PIN_VSYNC,
      .pin_href = 47,  // CAM_PIN_HREF,
      .pin_pclk = 13,  // CAM_PIN_PCLK,

      .xclk_freq_hz = xclk_freq_hz,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,

      .pixel_format = pixel_format,
      .frame_size = frame_size,

      .jpeg_quality = jpeg_quality,
      .fb_count = fb_count,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  };

  // initialize the camera sensor
  esp_err_t ret = esp_camera_init(&camera_config);
  if (ret != ESP_OK)
  {
    return ret;
  }

  // Get the sensor object, and then use some of its functions to adjust the parameters when taking a photo.
  // Note: Do not call functions that set resolution, set picture format and PLL clock,
  // If you need to reset the appeal parameters, please reinitialize the sensor.
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1); // flip it back
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_brightness(s, 1);  // up the blightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }

  if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID)
  {
    s->set_vflip(s, 1); // flip it back
  }
  else if (s->id.PID == GC0308_PID)
  {
    s->set_hmirror(s, 0);
  }
  else if (s->id.PID == GC032A_PID)
  {
    s->set_vflip(s, 1);
  }

  // Get the basic information of the sensor.
  camera_sensor_info_t *s_info = esp_camera_sensor_get_info(&(s->id));

  if (ESP_OK == ret && PIXFORMAT_JPEG == pixel_format && s_info->support_jpeg == true)
  {
    cur_xclk_freq_hz = xclk_freq_hz;
    cur_pixel_format = pixel_format;
    cur_frame_size = frame_size;
    cur_jpeg_quality = jpeg_quality;
    cur_fb_count = fb_count;
    inited = true;
  }
  else
  {
    ESP_LOGE(UVC_STREAM_TAG, "JPEG format is not supported");
    return ESP_ERR_NOT_SUPPORTED;
  }

  return ret;
}

static esp_err_t UVCStreamHelpers::camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx)
{
  (void)cb_ctx;
  ESP_LOGI(UVC_STREAM_TAG, "Camera Start");
  ESP_LOGI(UVC_STREAM_TAG, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);
  framesize_t frame_size = FRAMESIZE_QVGA;
  int jpeg_quality = 10;

  if (format != UVC_FORMAT_JPEG)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Only support MJPEG format");
    return ESP_ERR_NOT_SUPPORTED;
  }

  if (width == 240 && height == 240)
  {
    frame_size = FRAMESIZE_240X240;
    jpeg_quality = 10;
  }
  else
  {
    ESP_LOGE(UVC_STREAM_TAG, "Unsupported frame size %dx%d", width, height);
    return ESP_ERR_NOT_SUPPORTED;
  }

  esp_err_t ret = camera_init(20000000, PIXFORMAT_JPEG, frame_size, jpeg_quality, 2);
  if (ret != ESP_OK)
  {
    ESP_LOGE(UVC_STREAM_TAG, "camera init failed");
    return ret;
  }

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

  ESP_LOGI(UVC_STREAM_TAG, "Camera Stop");
}

static uvc_fb_t *UVCStreamHelpers::camera_fb_get_cb(void *cb_ctx)
{
  (void)cb_ctx;
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

  if (s_fb.uvc_fb.len > UVC_MAX_FRAMESIZE_SIZE)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Frame size %d is larger than max frame size %d", s_fb.uvc_fb.len, UVC_MAX_FRAMESIZE_SIZE);
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
  ESP_LOGI(UVC_STREAM_TAG, "Setting up UVC Stream");

  uvc_buffer = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);
  if (uvc_buffer == nullptr)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Allocating buffer for UVC Device failed");
    return ESP_FAIL;
  }

  uvc_device_config_t config = {
      .uvc_buffer = uvc_buffer,
      .uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE,
      .start_cb = UVCStreamHelpers::camera_start_cb,
      .fb_get_cb = UVCStreamHelpers::camera_fb_get_cb,
      .fb_return_cb = UVCStreamHelpers::camera_fb_return_cb,
      .stop_cb = UVCStreamHelpers::camera_stop_cb,
  };

  esp_err_t ret = uvc_device_config(0, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Configuring UVC Device failed: %s", esp_err_to_name(ret));
    return ret;
  }
  else
  {
    ESP_LOGI(UVC_STREAM_TAG, "Configured UVC Device");
  }

  ESP_LOGI(UVC_STREAM_TAG, "Initializing UVC Device");
  ret = uvc_device_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(UVC_STREAM_TAG, "Initializing UVC Device failed: %s", esp_err_to_name(ret));
    return ret;
  }
  else
  {
    ESP_LOGI(UVC_STREAM_TAG, "Initialized UVC Device");
  }

  return ESP_OK;
}