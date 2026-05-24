#pragma once

#include "Arduino.h"
#include <atomic>
#include <functional>
#include <thread>

class Ticker {
public:
  Ticker() = default;

  ~Ticker() {
    detach();
  }

  template <typename Callback>
  void once(float seconds, Callback callback) {
    scheduleOnce((unsigned long)(seconds * 1000.0f), std::function<void()>(callback));
  }

  template <typename Callback>
  void once_ms(unsigned long ms, Callback callback) {
    scheduleOnce(ms, std::function<void()>(callback));
  }

  template <typename Callback>
  void attach(float seconds, Callback callback) {
    scheduleRepeat((unsigned long)(seconds * 1000.0f), std::function<void()>(callback));
  }

  template <typename Callback>
  void attach_ms(unsigned long ms, Callback callback) {
    scheduleRepeat(ms, std::function<void()>(callback));
  }

  void detach() {
    _active = false;
  }

  bool active() const {
    return _active;
  }

private:
  std::atomic<bool> _active{false};

  void scheduleOnce(unsigned long ms, std::function<void()> callback) {
    _active = true;

    std::thread([this, ms, callback]() {
      delay(ms);
      if (_active && callback) callback();
      _active = false;
    }).detach();
  }

  void scheduleRepeat(unsigned long ms, std::function<void()> callback) {
    _active = true;

    std::thread([this, ms, callback]() {
      while (_active) {
        delay(ms);
        if (_active && callback) callback();
      }
    }).detach();
  }
};
