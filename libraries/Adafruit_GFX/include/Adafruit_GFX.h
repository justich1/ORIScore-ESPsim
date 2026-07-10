#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#ifndef BLACK
#define BLACK 0
#endif
#ifndef WHITE
#define WHITE 1
#endif
#ifndef INVERSE
#define INVERSE 2
#endif

#ifndef GFX_BLACK
#define GFX_BLACK 0x0000
#endif
#ifndef GFX_WHITE
#define GFX_WHITE 0xFFFF
#endif
#ifndef GFX_RED
#define GFX_RED 0xF800
#endif
#ifndef GFX_GREEN
#define GFX_GREEN 0x07E0
#endif
#ifndef GFX_BLUE
#define GFX_BLUE 0x001F
#endif

// Základní kompatibilní implementace Adafruit_GFX pro ORIScore ESPsim.
// Veškeré kreslení končí ve virtuální metodě drawPixel(), takže ho mohou
// použít SSD1306 i další simulované displeje.
class Adafruit_GFX : public Print {
public:
    int16_t WIDTH;
    int16_t HEIGHT;
    int16_t _width;
    int16_t _height;
    int16_t cursor_x;
    int16_t cursor_y;
    uint16_t textcolor;
    uint16_t textbgcolor;
    uint8_t textsize_x;
    uint8_t textsize_y;
    uint8_t rotation;
    bool wrap;

    Adafruit_GFX(int16_t w, int16_t h);
    virtual ~Adafruit_GFX() = default;

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

    virtual void startWrite() {}
    virtual void writePixel(int16_t x, int16_t y, uint16_t color);
    virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    virtual void endWrite() {}

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillScreen(uint16_t color);

    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t x2, int16_t y2, uint16_t color);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t color);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                       int16_t r, uint16_t color);

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                    int16_t w, int16_t h, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                    int16_t w, int16_t h, uint16_t color, uint16_t bg);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                     int16_t w, int16_t h, uint16_t color);

    void drawChar(int16_t x, int16_t y, unsigned char c,
                  uint16_t color, uint16_t bg, uint8_t size);
    void drawChar(int16_t x, int16_t y, unsigned char c,
                  uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);

    void setCursor(int16_t x, int16_t y);
    int16_t getCursorX() const;
    int16_t getCursorY() const;

    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void setTextSize(uint8_t s);
    void setTextSize(uint8_t sx, uint8_t sy);
    void setTextWrap(bool w);
    void cp437(bool x = true) { (void)x; }
    void setFont(const void* f = nullptr) { (void)f; }

    void setRotation(uint8_t r);
    uint8_t getRotation() const;
    int16_t width() const;
    int16_t height() const;

    using Print::write;
    size_t write(uint8_t c) override;

    void getTextBounds(const char* string, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);
    void getTextBounds(const String& string, int16_t x, int16_t y,
                       int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h);

protected:
    static void swapInt16(int16_t& a, int16_t& b);
};

class Adafruit_GFX_Button {
public:
    Adafruit_GFX_Button() = default;

    void initButton(Adafruit_GFX* gfx, int16_t x, int16_t y,
                    uint16_t w, uint16_t h, uint16_t outline,
                    uint16_t fill, uint16_t textcolor,
                    char* label, uint8_t textsize);
    void initButtonUL(Adafruit_GFX* gfx, int16_t x1, int16_t y1,
                      uint16_t w, uint16_t h, uint16_t outline,
                      uint16_t fill, uint16_t textcolor,
                      char* label, uint8_t textsize);
    void drawButton(bool inverted = false);
    bool contains(int16_t x, int16_t y);
    void press(bool p);
    bool isPressed();
    bool justPressed();
    bool justReleased();

private:
    Adafruit_GFX* _gfx = nullptr;
    int16_t _x1 = 0;
    int16_t _y1 = 0;
    uint16_t _w = 0;
    uint16_t _h = 0;
    uint8_t _textsize = 1;
    uint16_t _outlinecolor = WHITE;
    uint16_t _fillcolor = BLACK;
    uint16_t _textcolor = WHITE;
    char _label[10] = {0};
    bool currstate = false;
    bool laststate = false;
};
