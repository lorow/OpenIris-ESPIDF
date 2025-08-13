#pragma once
#ifndef _LEDMANAGER_HPP_
#define _LEDMANAGER_HPP_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef CONFIG_LED_EXTERNAL_CONTROL
#include "driver/ledc.h"
#endif

#include <esp_log.h>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <StateManager.hpp>
#include <helpers.hpp>

// it kinda looks like different boards have these states swapped
#define LED_OFF 1
#define LED_ON 0

struct BlinkPatterns_t
{
  int state;
  int delayTime;
};

struct LEDStage
{
  bool isError;
  bool isRepeatable;
  std::vector<BlinkPatterns_t> patterns;
};

typedef std::unordered_map<LEDStates_e, LEDStage>
    ledStateMap_t;

class LEDManager
{
public:
  LEDManager(gpio_num_t blink_led_pin, gpio_num_t illumninator_led_pin, QueueHandle_t ledStateQueue);

  void setup();
  void handleLED();
  size_t getTimeToDelayFor() const { return timeToDelayFor; }

private:
  void toggleLED(bool state) const;
  void displayCurrentPattern();
  void updateState(LEDStates_e newState);

  gpio_num_t blink_led_pin;
  gpio_num_t illumninator_led_pin;
  QueueHandle_t ledStateQueue;

  static ledStateMap_t ledStateMap;

  LEDStates_e buffer;
  LEDStates_e currentState;

  size_t currentPatternIndex = 0;
  size_t timeToDelayFor = 100;
  bool finishedPattern = false;
};

void HandleLEDDisplayTask(void *pvParameter);
#endif