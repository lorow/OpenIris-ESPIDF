#pragma once
#ifndef OPENIRISTASKS_HPP
#define OPENIRISTASKS_HPP

#include "Helpers.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

namespace OpenIrisTasks
{
  void ScheduleRestart(int milliseconds);
};

#endif