#include "LEDManager.hpp"

const char *LED_MANAGER_TAG = "[LED_MANAGER]";

ledStateMap_t LEDManager::ledStateMap = {
    {
        LEDStates_e::_LedStateNone,
        {
            false,
            true,
            {{LED_OFF, 1000}},
        },
    },
    {
        LEDStates_e::_LedStateStreaming,
        {
            false,
            true,
            {{LED_ON, 1000}},
        },
    },
    {
        LEDStates_e::_LedStateStoppedStreaming,
        {
            false,
            true,
            {{LED_OFF, 1000}},
        },
    },
    {
        LEDStates_e::_Camera_Error,
        {
            true,
            true,
            {{{LED_ON, 300}, {LED_OFF, 300}, {LED_ON, 300}, {LED_OFF, 300}}},
        },
    },
    {
        LEDStates_e::_WiFiState_Connecting,
        {
            false,
            true,
            {{LED_ON, 400}, {LED_OFF, 400}},
        },
    },
    {
        LEDStates_e::_WiFiState_Connected,
        {
            false,
            false,
            {{LED_ON, 200}, {LED_OFF, 200}, {LED_ON, 200}, {LED_OFF, 200}, {LED_ON, 200}, {LED_OFF, 200}, {LED_ON, 200}, {LED_OFF, 200}, {LED_ON, 200}, {LED_OFF, 200}},
        },
    },
    {
        LEDStates_e::_WiFiState_Error,
        {
            true,
            true,
            {{LED_ON, 200}, {LED_OFF, 100}, {LED_ON, 500}, {LED_OFF, 100}, {LED_ON, 200}},
        },
    },
};

LEDManager::LEDManager(gpio_num_t pin, gpio_num_t illumninator_led_pin, QueueHandle_t ledStateQueue) : blink_led_pin(pin),
                                                                                                       illumninator_led_pin(illumninator_led_pin), ledStateQueue(ledStateQueue), currentState(LEDStates_e::_LedStateNone)
{
}

void LEDManager::setup()
{
  ESP_LOGD(LED_MANAGER_TAG, "Setting up status led.");
  gpio_reset_pin(blink_led_pin);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(blink_led_pin, GPIO_MODE_OUTPUT);
  this->toggleLED(LED_OFF);

#ifdef CONFIG_SUPPORTS_EXTERNAL_LED_CONTROL
  ESP_LOGD(LED_MANAGER_TAG, "Setting up illuminator led.");
  const int freq = 5000;
  const auto resolution = LEDC_TIMER_8_BIT;
  const int dutyCycle = 255;

  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = resolution,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = freq,
      .clk_cfg = LEDC_AUTO_CLK};

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .gpio_num = this->illumninator_led_pin,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = dutyCycle,
      .hpoint = 0};

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
#endif
  ESP_LOGD(LED_MANAGER_TAG, "Done.");
}

void LEDManager::handleLED()
{
  if (!this->finishedPattern)
  {
    displayCurrentPattern();
    return;
  }

  if (xQueueReceive(this->ledStateQueue, &buffer, 10))
  {
    this->updateState(buffer);
  }
  else
  {
    // we've finished displaying the pattern, so let's check if it's repeatable and if so - reset it
    if (this->ledStateMap[this->currentState].isRepeatable || this->ledStateMap[this->currentState].isError)
    {
      this->currentPatternIndex = 0;
      this->finishedPattern = false;
    }
  }
}

void LEDManager::displayCurrentPattern()
{
  BlinkPatterns_t ledState = this->ledStateMap[this->currentState].patterns[this->currentPatternIndex];
  this->toggleLED(ledState.state);
  this->timeToDelayFor = ledState.delayTime;

  if (!(this->currentPatternIndex >= this->ledStateMap[this->currentState].patterns.size() - 1))
    this->currentPatternIndex++;
  else
  {
    this->finishedPattern = true;
    this->toggleLED(LED_OFF);
  }
}

void LEDManager::updateState(LEDStates_e newState)
{
  // we should change the displayed state
  // only if we finished displaying the current one - which is handled by the task
  // if the new state is not the same as the current one
  // and if can actually display the state

  // if we've got an error state - that's it, we'll just keep repeating it indefinitely
  if (this->ledStateMap[this->currentState].isError)
    return;

  if (newState == this->currentState)
    return;

  if (this->ledStateMap.contains(newState))
  {
    this->currentState = newState;
    this->currentPatternIndex = 0;
    this->finishedPattern = false;
  }
}

void LEDManager::toggleLED(bool state) const
{
  gpio_set_level(blink_led_pin, state);
}

void HandleLEDDisplayTask(void *pvParameter)
{
  LEDManager *ledManager = static_cast<LEDManager *>(pvParameter);

  while (1)
  {
    ledManager->handleLED();
    vTaskDelay(ledManager->getTimeToDelayFor());
  }
}