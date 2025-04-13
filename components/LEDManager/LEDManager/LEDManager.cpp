#include "LEDManager.hpp"

const char *LED_MANAGER_TAG = "[LED_MANAGER]";

LEDManager::ledStateMap_t LEDManager::ledStateMap = {
    {LEDStates_e::_LedStateNone, {{0, 500}}},
    {LEDStates_e::_Improv_Error,
     {{1, 1000}, {0, 500}, {0, 1000}, {0, 500}, {1, 1000}}},
    {LEDStates_e::_Improv_Start,
     {{1, 500}, {0, 300}, {0, 300}, {0, 300}, {1, 500}}},
    {LEDStates_e::_Improv_Stop,
     {{1, 300}, {0, 500}, {0, 500}, {0, 500}, {1, 300}}},
    {LEDStates_e::_Improv_Processing,
     {{1, 200}, {0, 100}, {0, 500}, {0, 100}, {1, 200}}},
    {LEDStates_e::_WebServerState_Error,
     {{1, 200}, {0, 100}, {0, 500}, {0, 100}, {1, 200}}},
    {LEDStates_e::_WiFiState_Error,
     {{1, 200}, {0, 100}, {0, 500}, {0, 100}, {1, 200}}},
    {LEDStates_e::_MDNSState_Error,
     {{1, 200},
      {0, 100},
      {1, 200},
      {0, 100},
      {0, 500},
      {0, 100},
      {1, 200},
      {0, 100},
      {1, 200}}},
    {LEDStates_e::_Camera_Error,
     {{1, 5000}}}, // this also works as a more general error - something went
                   // critically wrong? We go here
    {LEDStates_e::_WiFiState_Connecting, {{1, 100}, {0, 100}}},
    {LEDStates_e::_WiFiState_Connected,
     {{1, 100},
      {0, 100},
      {1, 100},
      {0, 100},
      {1, 100},
      {0, 100},
      {1, 100},
      {0, 100},
      {1, 100},
      {0, 100}}}};

std::vector<LEDStates_e> LEDManager::keepAliveStates = {
    LEDStates_e::_WebServerState_Error, LEDStates_e::_Camera_Error};

LEDManager::LEDManager(gpio_num_t pin, gpio_num_t illumninator_led_pin) : blink_led_pin(pin), illumninator_led_pin(illumninator_led_pin), state(false) {}

void LEDManager::setup()
{
  ESP_LOGD(LED_MANAGER_TAG, "Setting up status led.");
  gpio_reset_pin(blink_led_pin);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(blink_led_pin, GPIO_MODE_OUTPUT);
  // the defualt state is _LedStateNone so we're fine
  this->currentState = ledStateManager.getCurrentState();
  this->currentPatternIndex = 0;
  BlinkPatterns_t pattern =
      this->ledStateMap[this->currentState][this->currentPatternIndex];
  this->toggleLED(pattern.state);
  this->nextStateChangeMillis = pattern.delayTime;

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

/**
 * @brief Display the current state of the LED manager as a pattern of blinking
 * LED
 * @details This function must be called in the main loop
 */
void LEDManager::handleLED()
{
  if (Helpers::getTimeInMillis() <= this->nextStateChangeMillis)
  {
    return;
  }

  // !TODO what if we want a looping state? Or a state that needs to stay
  // bright? Am overthinking this, aren't I?

  // we've reached the timeout on that state, check if we can grab next one and
  // start displaying it, or if we need to keep displaying the current one
  if (this->currentPatternIndex >
      this->ledStateMap[this->currentState].size() - 1)
  {
    auto nextState = ledStateManager.getCurrentState();
    // we want to keep displaying the same state only if its an keepAlive one,
    // but we should change if the incoming one is also an errours state, maybe
    // more serious one this time <- this may be a bad idea
    if ((std::find(this->keepAliveStates.begin(), this->keepAliveStates.end(),
                   this->currentState) != this->keepAliveStates.end() ||
         std::find(this->keepAliveStates.begin(), this->keepAliveStates.end(),
                   nextState) != this->keepAliveStates.end()) ||
        (this->currentState != nextState &&
         this->ledStateMap.find(nextState) != this->ledStateMap.end()))
    {
      ESP_LOGD(LED_MANAGER_TAG, "Updating the state and reseting");
      this->toggleLED(false);
      this->currentState = nextState;
      this->currentPatternIndex = 0;
      BlinkPatterns_t pattern =
          this->ledStateMap[this->currentState][this->currentPatternIndex];
      this->nextStateChangeMillis = Helpers::getTimeInMillis() + pattern.delayTime;
      return;
    }
    // it wasn't a keepAlive state, nor did we have another one ready,
    // we're done for now
    this->toggleLED(false);
    return;
  }
  // we can safely advance it and display the next stage
  BlinkPatterns_t pattern =
      this->ledStateMap[this->currentState][this->currentPatternIndex];
  this->toggleLED(pattern.state);
  this->nextStateChangeMillis = Helpers::getTimeInMillis() + pattern.delayTime;
  ESP_LOGD(LED_MANAGER_TAG, "before updating stage %d", this->currentPatternIndex);
  this->currentPatternIndex++;
  ESP_LOGD(LED_MANAGER_TAG, "updated stage %d", this->currentPatternIndex);
}

/**
 * @brief Turn the LED on or off
 *
 * @param state
 */
void LEDManager::toggleLED(bool state) const
{
  gpio_set_level(blink_led_pin, state);
}
