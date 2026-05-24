#pragma once

#include "Arduino.h"
#include <cstdint>

#ifndef ESP_OK
#define ESP_OK 0
#endif

#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif

typedef int esp_err_t;

typedef enum {
  ESP_SLEEP_WAKEUP_UNDEFINED = 0,
  ESP_SLEEP_WAKEUP_ALL,
  ESP_SLEEP_WAKEUP_EXT0,
  ESP_SLEEP_WAKEUP_EXT1,
  ESP_SLEEP_WAKEUP_TIMER,
  ESP_SLEEP_WAKEUP_TOUCHPAD,
  ESP_SLEEP_WAKEUP_ULP,
  ESP_SLEEP_WAKEUP_GPIO,
  ESP_SLEEP_WAKEUP_UART,
  ESP_SLEEP_WAKEUP_WIFI,
  ESP_SLEEP_WAKEUP_COCPU,
  ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG,
  ESP_SLEEP_WAKEUP_BT
} esp_sleep_wakeup_cause_t;

typedef enum {
  ESP_EXT1_WAKEUP_ALL_LOW = 0,
  ESP_EXT1_WAKEUP_ANY_HIGH = 1
} esp_sleep_ext1_wakeup_mode_t;

inline esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us) {
  Serial.println(
    String("[SIM] esp_sleep_enable_timer_wakeup ") +
    String((unsigned long)(time_in_us / 1000ULL)) +
    " ms"
  );
  return ESP_OK;
}

inline esp_err_t esp_sleep_enable_ext0_wakeup(int gpio_num, int level) {
  Serial.println(
    String("[SIM] esp_sleep_enable_ext0_wakeup GPIO") +
    String(gpio_num) +
    " level=" +
    String(level)
  );
  return ESP_OK;
}

inline esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t mask, esp_sleep_ext1_wakeup_mode_t mode) {
  Serial.println(
    String("[SIM] esp_sleep_enable_ext1_wakeup mask=") +
    String((unsigned long)mask) +
    " mode=" +
    String((int)mode)
  );
  return ESP_OK;
}

inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() {
  return ESP_SLEEP_WAKEUP_UNDEFINED;
}

inline void esp_deep_sleep_start() {
  Serial.println("[SIM] esp_deep_sleep_start");

  // V simulátoru deep sleep zatím neukončuje proces.
  // Jen uděláme pseudo restart firmware přes ESP.restart().
  ESP.restart();
}

inline void esp_light_sleep_start() {
  Serial.println("[SIM] esp_light_sleep_start");
}