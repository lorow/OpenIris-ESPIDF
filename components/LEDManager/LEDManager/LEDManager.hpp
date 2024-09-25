#pragma once
#ifndef _LEDMANAGER_HPP_
#define _LEDMANAGER_HPP_

#include "driver/gpio.h"

#ifdef CONFIG_USE_ILLUMNATIOR_PIN
#include "driver/ledc.h"
#endif

#include <esp_log.h>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <StateManager.hpp>
#include <Helpers.hpp>

class LEDManager
{
public:
  LEDManager(gpio_num_t blink_led_pin);

#ifdef CONFIG_USE_ILLUMNATIOR_PIN
  gpio_num_t illumninator_led_pin;
  LEDManager(gpio_num_t blink_led_pin, gpio_num_t illumninator_led_pin);
#endif

  void setup();
  void handleLED();

private:
  gpio_num_t blink_led_pin;
  unsigned long nextStateChangeMillis = 0;
  bool state = false;

  struct BlinkPatterns_t
  {
    int state;
    int delayTime;
  };

  typedef std::unordered_map<LEDStates_e, std::vector<BlinkPatterns_t>> ledStateMap_t;
  static ledStateMap_t ledStateMap;
  static std::vector<LEDStates_e> keepAliveStates;
  LEDStates_e currentState;
  unsigned int currentPatternIndex = 0;

  void toggleLED(bool state) const;
};

#endif