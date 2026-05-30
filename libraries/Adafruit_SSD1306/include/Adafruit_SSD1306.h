#pragma once

/*
  Adafruit_SSD1306.h pro ORIScore ESPsim
  --------------------------------------
  Lehká kompatibilní náhrada Adafruit_SSD1306.

  Rozdíl proti původnímu stubu:
  - display() neposílá nic přes Wire => žádný spam Wire.endTransmission addr=0x3C
  - clearDisplay() opravdu maže framebuffer
  - drawPixel() opravdu zapisuje do framebufferu
  - getBuffer() vrací skutečný framebuffer
  - podporuje Adafruit_GFX text/kreslení přes drawPixel()

  Tohle je vhodné pro simulátor.
  Na reálný ESP používej normální knihovnu Adafruit_SSD1306.
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <stdint.h>
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
          _inverted(false)
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
          _inverted(false)
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
            // V simulátoru může Wire.begin() jen oznámit inicializaci.
            // Display data přes Wire neposíláme.
            _wire->begin();
        }

        _bufferSize = ((_w + 7) / 8) * _h;
        // Adafruit SSD1306 interně používá velikost W * ((H + 7) / 8).
        // Pro kompatibilitu getBuffer() držíme stejný layout:
        _bufferSize = (size_t)_w * ((_h + 7) / 8);

        if (_buffer) {
            delete[] _buffer;
            _buffer = nullptr;
        }

        _buffer = new uint8_t[_bufferSize];
        if (!_buffer) {
            _bufferSize = 0;
            return false;
        }

        clearDisplay();
        return true;
    }

    void display() {
        // Záměrně neposílat framebuffer přes Wire.
        // ORIS sim si případně může vzít data přes getBuffer().
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
        if (x < 0 || y < 0 || x >= _w || y >= _h) return;

        // Adafruit buffer layout:
        // index = x + (y / 8) * width
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

    int16_t width(void) const {
        return _w;
    }

    int16_t height(void) const {
        return _h;
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
};
