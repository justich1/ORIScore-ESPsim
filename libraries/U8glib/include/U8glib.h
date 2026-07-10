#pragma once

#include "Arduino.h"
#include <cstdint>
#include <string>
#include <vector>

#ifndef U8G_I2C_OPT_NONE
#define U8G_I2C_OPT_NONE 0
#endif
#ifndef U8G_I2C_OPT_DEV_0
#define U8G_I2C_OPT_DEV_0 0
#endif
#ifndef U8G_I2C_OPT_DEV_1
#define U8G_I2C_OPT_DEV_1 1
#endif
#ifndef U8G_I2C_OPT_FAST
#define U8G_I2C_OPT_FAST 16
#endif
#ifndef U8G_PROGMEM
#define U8G_PROGMEM PROGMEM
#endif

extern const uint8_t u8g_font_6x10[];
extern const uint8_t u8g_font_5x7[];
extern const uint8_t u8g_font_unifont[];

class U8GLIB {
public:
  U8GLIB(int width, int height, const char* controller, const char* iface);
  virtual ~U8GLIB() = default;

  void begin();
  bool firstPage();
  bool nextPage();

  void setFont(const uint8_t* font);
  void setColorIndex(uint8_t color);
  void setPrintPos(int x, int y);

  void drawPixel(int x, int y);
  void drawLine(int x0, int y0, int x1, int y1);
  void drawHLine(int x, int y, int w);
  void drawVLine(int x, int y, int h);
  void drawBox(int x, int y, int w, int h);
  void drawFrame(int x, int y, int w, int h);
  void drawStr(int x, int y, const char* s);
  void drawStr(int x, int y, const String& s);

  size_t print(const char* s);
  size_t print(const String& s);
  size_t print(char c);
  size_t print(int n);
  size_t print(unsigned int n);
  size_t print(long n);
  size_t print(unsigned long n);
  size_t println(const char* s);
  size_t println(const String& s);

  void clear();
  void sleepOn();
  void sleepOff();
  void setRot180();
  void undoRotation();

  int getWidth() const;
  int getHeight() const;

protected:
  int _width;
  int _height;
  std::string _controller;
  std::string _iface;
  std::vector<uint8_t> _buffer;
  std::vector<uint8_t> _lastEmittedBuffer;
  bool _hasLastEmittedFrame = false;
  uint8_t _color = 1;
  int _printX = 0;
  int _printY = 8;
  bool _pageActive = false;
  bool _sleep = false;

  void putPixel(int x, int y, bool on);
  void drawChar5x7(int x, int y, char c);
  void emitConfig();
  void emitFrame();
};

class U8GLIB_SSD1306_128X64 : public U8GLIB {
public:
  explicit U8GLIB_SSD1306_128X64(uint8_t options = U8G_I2C_OPT_NONE);
  U8GLIB_SSD1306_128X64(uint8_t cs, uint8_t dc, uint8_t reset);
  U8GLIB_SSD1306_128X64(uint8_t sck, uint8_t mosi, uint8_t cs, uint8_t dc, uint8_t reset);
};

class U8GLIB_SSD1306_128X32 : public U8GLIB {
public:
  explicit U8GLIB_SSD1306_128X32(uint8_t options = U8G_I2C_OPT_NONE);
  U8GLIB_SSD1306_128X32(uint8_t cs, uint8_t dc, uint8_t reset);
  U8GLIB_SSD1306_128X32(uint8_t sck, uint8_t mosi, uint8_t cs, uint8_t dc, uint8_t reset);
};

// Pomocné API pro budoucí rozšíření konfigurace z UI.
void simDisplayConfigure(const String& controller, const String& iface, int width, int height);
