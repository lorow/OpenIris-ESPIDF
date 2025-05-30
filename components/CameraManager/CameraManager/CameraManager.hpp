#pragma once
#ifndef CAMERAMANAGER_HPP
#define CAMERAMANAGER_HPP

#include "esp_log.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "esp_psram.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <StateManager.hpp>
#include <ProjectConfig.hpp>

#ifndef DEFAULT_XCLK_FREQ_HZ
#define DEFAULT_XCLK_FREQ_HZ 16500000
#define USB_DEFAULT_XCLK_FREQ_HZ 20000000
#define OV5640_XCLK_FREQ_HZ DEFAULT_XCLK_FREQ_HZ
#endif

class CameraManager
{
private:
  sensor_t *camera_sensor;
  std::shared_ptr<ProjectConfig> projectConfig;
  QueueHandle_t eventQueue;
  camera_config_t config;

public:
  CameraManager(std::shared_ptr<ProjectConfig> projectConfig, QueueHandle_t eventQueue);
  int setCameraResolution(framesize_t frameSize);
  bool setupCamera();
  int setVFlip(int direction);
  int setHFlip(int direction);
  int setVieWindow(int offsetX, int offsetY, int outputX, int outputY);
  void resetCamera(bool type);

private:
  void loadConfigData();
  void setupCameraPinout();
  void setupCameraSensor();
  void setupBasicResolution();
};

#endif // CAMERAMANAGER_HPP