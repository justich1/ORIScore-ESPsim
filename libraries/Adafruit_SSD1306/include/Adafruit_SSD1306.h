#pragma once

/*
  Adafruit_SSD1306 -> U8glib bridge pro ORIScore ESPsim
  -----------------------------------------------------
  Cíl:
  - Sketch může používat Adafruit_SSD1306 API.
  - Simulátor ORIS dostane obraz přes U8glib, takže funguje náhled displeje.
  - display() neposílá framebuffer přes Wire, takže nespamuje Wire.endTransmission addr=0x3C.
  - drawPixel() zapisuje do framebufferu.
  - Adafruit_GFX text/kreslení funguje přes drawPixel().
  - getBuffer() vrací skutečný framebuffer.

  Používej jen v ORIS ESPsim.
  Na reálný ESP použij originální Adafruit_SSD1306 knihovnu.
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <U8glib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef SSD1306_BLACK
#define SSD1306_BLACK 0
#endif

#ifndef SSD1306_WHITE
#define SSD1306_WHITE 1
#endif

#ifndef SSD1306_INVERSE
#define SSD1306_INVERSE 2
#endif

#ifndef SSD1306_SWITCHCAPVCC
#define SSD1306_SWITCHCAPVCC 0x2
#endif

#ifndef SSD1306_EXTERNALVCC
#define SSD1306_EXTERNALVCC 0x1
#endif

#ifndef SSD1306_MEMORYMODE
#define SSD1306_MEMORYMODE 0x20
#endif

#ifndef SSD1306_COLUMNADDR
#define SSD1306_COLUMNADDR 0x21
#endif

#ifndef SSD1306_PAGEADDR
#define SSD1306_PAGEADDR 0x22
#endif

#ifndef SSD1306_SETCONTRAST
#define SSD1306_SETCONTRAST 0x81
#endif

#ifndef SSD1306_DISPLAYALLON_RESUME
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#endif

#ifndef SSD1306_DISPLAYALLON
#define SSD1306_DISPLAYALLON 0xA5
#endif

#ifndef SSD1306_NORMALDISPLAY
#define SSD1306_NORMALDISPLAY 0xA6
#endif

#ifndef SSD1306_INVERTDISPLAY
#define SSD1306_INVERTDISPLAY 0xA7
#endif

#ifndef SSD1306_DISPLAYOFF
#define SSD1306_DISPLAYOFF 0xAE
#endif

#ifndef SSD1306_DISPLAYON
#define SSD1306_DISPLAYON 0xAF
#endif

#ifndef SSD1306_SETDISPLAYCLOCKDIV
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#endif

#ifndef SSD1306_SETMULTIPLEX
#define SSD1306_SETMULTIPLEX 0xA8
#endif

#ifndef SSD1306_SETDISPLAYOFFSET
#define SSD1306_SETDISPLAYOFFSET 0xD3
#endif

#ifndef SSD1306_SETSTARTLINE
#define SSD1306_SETSTARTLINE 0x40
#endif

#ifndef SSD1306_CHARGEPUMP
#define SSD1306_CHARGEPUMP 0x8D
#endif

#ifndef SSD1306_SEGREMAP
#define SSD1306_SEGREMAP 0xA0
#endif

#ifndef SSD1306_COMSCANDEC
#define SSD1306_COMSCANDEC 0xC8
#endif

#ifndef SSD1306_SETCOMPINS
#define SSD1306_SETCOMPINS 0xDA
#endif

#ifndef SSD1306_SETPRECHARGE
#define SSD1306_SETPRECHARGE 0xD9
#endif

#ifndef SSD1306_SETVCOMDETECT
#define SSD1306_SETVCOMDETECT 0xDB
#endif

class Adafruit_SSD1306 : public Adafruit_GFX {
public:
    Adafruit_SSD1306(int16_t w, int16_t h, TwoWire* twi = &Wire, int8_t rst_pin = -1,
                     uint32_t clkDuring = 400000UL, uint32_t clkAfter = 100000UL)
        : Adafruit_GFX(w, h),
          _w(w),
          _h(h),
          _wire(twi),
          _rst(rst_pin),
          _buffer(nullptr),
          _bufferSize(0),
          _addr(0x3C),
          _inverted(false),
          _begun(false)
    {
        (void)clkDuring;
        (void)clkAfter;
    }

    Adafruit_SSD1306(int8_t rst_pin = -1)
        : Adafruit_GFX(128, 64),
          _w(128),
          _h(64),
          _wire(&Wire),
          _rst(rst_pin),
          _buffer(nullptr),
          _bufferSize(0),
          _addr(0x3C),
          _inverted(false),
          _begun(false)
    {
    }

    ~Adafruit_SSD1306() {
        if (_buffer) {
            delete[] _buffer;
            _buffer = nullptr;
        }
    }

    bool begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = 0,
               bool reset = true, bool periphBegin = true) {
        (void)switchvcc;
        (void)reset;

        if (i2caddr != 0) {
            _addr = i2caddr;
        } else {
            _addr = 0x3C;
        }

        if (periphBegin && _wire) {
            _wire->begin();
        }

        _bufferSize = (size_t)_w * ((_h + 7) / 8);

        if (_buffer) {
            delete[] _buffer;
            _buffer = nullptr;
        }

        _buffer = new uint8_t[_bufferSize];
        if (!_buffer) {
            _bufferSize = 0;
            _begun = false;
            return false;
        }

        clearDisplay();
        _begun = true;
        return true;
    }

    void display() {
        if (!_begun || !_buffer) return;

        U8GLIB_SSD1306_128X64& u = getU8g();

        u.firstPage();
        do {
            u.setColorIndex(1);

            for (int16_t y = 0; y < _h; y++) {
                for (int16_t x = 0; x < _w; x++) {
                    if (bufferPixel(x, y)) {
                        u.drawPixel(x, y);
                    }
                }
            }
        } while (u.nextPage());
    }

    void clearDisplay() {
        if (_buffer && _bufferSize) {
            memset(_buffer, 0x00, _bufferSize);
        }
    }

    void invertDisplay(bool i) {
        _inverted = i;
    }

    void dim(bool dim) {
        (void)dim;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (!_buffer) return;

        // Jednoduché zachování kompatibility s rotací GFX.
        switch (getRotation()) {
            case 1:
                swapInt16(x, y);
                x = WIDTH - x - 1;
                break;

            case 2:
                x = WIDTH - x - 1;
                y = HEIGHT - y - 1;
                break;

            case 3:
                swapInt16(x, y);
                y = HEIGHT - y - 1;
                break;
        }

        if (x < 0 || y < 0 || x >= _w || y >= _h) return;

        size_t index = (size_t)x + (size_t)(y / 8) * (size_t)_w;
        if (index >= _bufferSize) return;

        uint8_t mask = (uint8_t)(1 << (y & 7));

        switch (color) {
            case SSD1306_WHITE:
                _buffer[index] |= mask;
                break;

            case SSD1306_BLACK:
                _buffer[index] &= ~mask;
                break;

            case SSD1306_INVERSE:
                _buffer[index] ^= mask;
                break;

            default:
                if (color) _buffer[index] |= mask;
                else _buffer[index] &= ~mask;
                break;
        }
    }

    void startscrollright(uint8_t start, uint8_t stop) {
        (void)start;
        (void)stop;
    }

    void startscrollleft(uint8_t start, uint8_t stop) {
        (void)start;
        (void)stop;
    }

    void startscrolldiagright(uint8_t start, uint8_t stop) {
        (void)start;
        (void)stop;
    }

    void startscrolldiagleft(uint8_t start, uint8_t stop) {
        (void)start;
        (void)stop;
    }

    void stopscroll() {
    }

    void ssd1306_command(uint8_t c) {
        (void)c;
    }

    void ssd1306_command1(uint8_t c) {
        (void)c;
    }

    void ssd1306_commandList(const uint8_t* c, uint8_t n) {
        (void)c;
        (void)n;
    }

    uint8_t* getBuffer() {
        static uint8_t dummy[1024] = {0};
        if (_buffer) return _buffer;
        return dummy;
    }

    const uint8_t* getBuffer() const {
        static uint8_t dummy[1024] = {0};
        if (_buffer) return _buffer;
        return dummy;
    }

private:
    int16_t _w;
    int16_t _h;
    TwoWire* _wire;
    int8_t _rst;
    uint8_t* _buffer;
    size_t _bufferSize;
    uint8_t _addr;
    bool _inverted;
    bool _begun;

    static void swapInt16(int16_t& a, int16_t& b) {
        int16_t t = a;
        a = b;
        b = t;
    }

    static U8GLIB_SSD1306_128X64& getU8g() {
        static U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
        return u8g;
    }

    bool bufferPixel(int16_t x, int16_t y) const {
        if (!_buffer) return false;
        if (x < 0 || y < 0 || x >= _w || y >= _h) return false;

        size_t index = (size_t)x + (size_t)(y / 8) * (size_t)_w;
        if (index >= _bufferSize) return false;

        uint8_t mask = (uint8_t)(1 << (y & 7));
        bool on = (_buffer[index] & mask) != 0;

        return _inverted ? !on : on;
    }
};
