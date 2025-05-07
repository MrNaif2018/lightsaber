#pragma once
#include <Arduino.h>
#include <WebSerial.h>

template <typename... Args>
size_t d_printf(const char *format, Args &&...args) {
  size_t ret = Serial.printf(format, std::forward<Args>(args)...);
  WebSerial.printf(format, std::forward<Args>(args)...);
  return ret;
}

template <typename T> size_t d_print(const T &v) {
  size_t ret = Serial.print(v);
  WebSerial.print(v);
  return ret;
}

template <typename T> size_t d_println(const T &v) {
  size_t ret = Serial.println(v);
  WebSerial.println(v);
  return ret;
}