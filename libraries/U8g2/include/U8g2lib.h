#pragma once

#include "Arduino.h"
#include <cstdint>
#include <string>
#include <vector>

#ifndef U8X8_PIN_NONE
#define U8X8_PIN_NONE 255
#endif

#ifndef U8G2_R0
#define U8G2_R0 0
#endif

extern const uint8_t u8g2_font_6x10_tf[];
extern const uint8_t u8g2_font_fub14_tr[];
extern const uint8_t u8g2_font_ncenB08_tr[];
extern const uint8_t u8g2_font_ncenB10_tr[];

class U8G2 {
public:
  U8G2(int width, int height, const char* controller, const char* iface);

  void begin();
  void clearBuffer();
  void sendBuffer();

  void setFont(const uint8_t* font);
  void setColorIndex(uint8_t color);
  void setPowerSave(uint8_t save);

  void setCursor(int x, int y);

  void drawPixel(int x, int y);
  void drawLine(int x0, int y0, int x1, int y1);
  void drawHLine(int x, int y, int w);
  void drawVLine(int x, int y, int h);
  void drawBox(int x, int y, int w, int h);
  void drawFrame(int x, int y, int w, int h);

  void drawStr(int x, int y, const char* s);
  void drawStr(int x, int y, const String& s);
  void drawUTF8(int x, int y, const char* s);
  void drawUTF8(int x, int y, const String& s);

  size_t print(const char* s);
  size_t print(const String& s);
  size_t print(char c);
  size_t print(int n);
  size_t print(unsigned int n);
  size_t print(long n);
  size_t print(unsigned long n);
  size_t print(float f);
  size_t print(float f, int decimals);
  size_t print(double f);
  size_t print(double f, int decimals);

  int getDisplayWidth() const;
  int getDisplayHeight() const;

protected:
  int _width;
  int _height;
  std::string _controller;
  std::string _iface;
  std::vector<uint8_t> _buffer;
  std::vector<uint8_t> _lastEmittedBuffer;
  bool _hasLastEmittedFrame = false;
  uint8_t _color = 1;
  int _cursorX = 0;
  int _cursorY = 8;
  bool _sleep = false;

  void putPixel(int x, int y, bool on);
  void drawChar5x7(int x, int y, char c);
  void emitConfig();
  void emitFrame();
};

class U8G2_SSD1306_128X64_NONAME_F_HW_I2C : public U8G2 {
public:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C(uint8_t rotation = U8G2_R0,
                                      uint8_t reset = U8X8_PIN_NONE,
                                      uint8_t clock = U8X8_PIN_NONE,
                                      uint8_t data = U8X8_PIN_NONE);
};

class U8G2_SSD1306_128X64_NONAME_F_SW_I2C : public U8G2 {
public:
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C(uint8_t rotation = U8G2_R0,
                                      uint8_t clock = U8X8_PIN_NONE,
                                      uint8_t data = U8X8_PIN_NONE,
                                      uint8_t reset = U8X8_PIN_NONE);
};

class U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C : public U8G2 {
public:
  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(uint8_t rotation = U8G2_R0,
                                         uint8_t reset = U8X8_PIN_NONE,
                                         uint8_t clock = U8X8_PIN_NONE,
                                         uint8_t data = U8X8_PIN_NONE);
};

class U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C : public U8G2 {
public:
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C(uint8_t rotation = U8G2_R0,
                                         uint8_t clock = U8X8_PIN_NONE,
                                         uint8_t data = U8X8_PIN_NONE,
                                         uint8_t reset = U8X8_PIN_NONE);
};
