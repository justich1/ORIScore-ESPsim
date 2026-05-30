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

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef ORISIM_GFXFONT_TYPES
#define ORISIM_GFXFONT_TYPES

typedef struct {
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    uint8_t xAdvance;
    int8_t xOffset;
    int8_t yOffset;
} GFXglyph;

typedef struct {
    uint8_t* bitmap;
    GFXglyph* glyph;
    uint8_t first;
    uint8_t last;
    uint8_t yAdvance;
} GFXfont;

#endif

class Adafruit_GFX {
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

    Adafruit_GFX(int16_t w, int16_t h)
        : WIDTH(w),
          HEIGHT(h),
          _width(w),
          _height(h),
          cursor_x(0),
          cursor_y(0),
          textcolor(WHITE),
          textbgcolor(BLACK),
          textsize_x(1),
          textsize_y(1),
          rotation(0),
          wrap(true)
    {
    }

    virtual ~Adafruit_GFX() {}

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) {
        (void)x;
        (void)y;
        (void)color;
    }

    virtual void startWrite() {}
    virtual void writePixel(int16_t x, int16_t y, uint16_t color) {
        drawPixel(x, y, color);
    }
    virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        fillRect(x, y, w, h, color);
    }
    virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        drawFastVLine(x, y, h, color);
    }
    virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        drawFastHLine(x, y, w, color);
    }
    virtual void endWrite() {}

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        (void)x;
        (void)y;
        (void)h;
        (void)color;
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        (void)x;
        (void)y;
        (void)w;
        (void)color;
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        (void)x0;
        (void)y0;
        (void)x1;
        (void)y1;
        (void)color;
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)color;
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)color;
    }

    void fillScreen(uint16_t color) {
        (void)color;
    }

    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        (void)x0;
        (void)y0;
        (void)r;
        (void)color;
    }

    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        (void)x0;
        (void)y0;
        (void)r;
        (void)color;
    }

    void drawTriangle(
        int16_t x0, int16_t y0,
        int16_t x1, int16_t y1,
        int16_t x2, int16_t y2,
        uint16_t color
    ) {
        (void)x0;
        (void)y0;
        (void)x1;
        (void)y1;
        (void)x2;
        (void)y2;
        (void)color;
    }

    void fillTriangle(
        int16_t x0, int16_t y0,
        int16_t x1, int16_t y1,
        int16_t x2, int16_t y2,
        uint16_t color
    ) {
        (void)x0;
        (void)y0;
        (void)x1;
        (void)y1;
        (void)x2;
        (void)y2;
        (void)color;
    }

    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)r;
        (void)color;
    }

    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)r;
        (void)color;
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
        (void)x;
        (void)y;
        (void)bitmap;
        (void)w;
        (void)h;
        (void)color;
    }

    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg) {
        (void)x;
        (void)y;
        (void)bitmap;
        (void)w;
        (void)h;
        (void)color;
        (void)bg;
    }

    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
        (void)x;
        (void)y;
        (void)bitmap;
        (void)w;
        (void)h;
        (void)color;
    }

    void setCursor(int16_t x, int16_t y) {
        cursor_x = x;
        cursor_y = y;
    }

    int16_t getCursorX() const {
        return cursor_x;
    }

    int16_t getCursorY() const {
        return cursor_y;
    }

    void setTextColor(uint16_t c) {
        textcolor = c;
        textbgcolor = c;
    }

    void setTextColor(uint16_t c, uint16_t bg) {
        textcolor = c;
        textbgcolor = bg;
    }

    void setTextSize(uint8_t s) {
        textsize_x = s;
        textsize_y = s;
    }

    void setTextSize(uint8_t sx, uint8_t sy) {
        textsize_x = sx;
        textsize_y = sy;
    }

    void setTextWrap(bool w) {
        wrap = w;
    }

    void setRotation(uint8_t r) {
        rotation = r & 3;
    }

    uint8_t getRotation() const {
        return rotation;
    }

    int16_t width() const {
        return _width;
    }

    int16_t height() const {
        return _height;
    }

    size_t write(uint8_t c) {
        (void)c;
        return 1;
    }

    size_t print(const char* s) {
        if (!s) return 0;
        size_t n = 0;
        while (s[n]) n++;
        return n;
    }

    size_t print(const String& s) {
        return s.length();
    }

    size_t print(int v) {
        (void)v;
        return 1;
    }

    size_t print(float v) {
        (void)v;
        return 1;
    }

    size_t println() {
        return 1;
    }

    size_t println(const char* s) {
        return print(s) + 1;
    }

    size_t println(const String& s) {
        return print(s) + 1;
    }

    size_t println(int v) {
        return print(v) + 1;
    }

    size_t println(float v) {
        return print(v) + 1;
    }

    void getTextBounds(
        const char* string,
        int16_t x,
        int16_t y,
        int16_t* x1,
        int16_t* y1,
        uint16_t* w,
        uint16_t* h
    ) {
        if (x1) *x1 = x;
        if (y1) *y1 = y;
        if (w) {
            uint16_t len = 0;
            if (string) {
                while (string[len]) len++;
            }
            *w = len * 6 * textsize_x;
        }
        if (h) *h = 8 * textsize_y;
    }

void setFont(const GFXfont* f = nullptr) {
    (void)f;
}

};

class Adafruit_GFX_Button {
public:
    Adafruit_GFX_Button() {}

    void initButton(
        Adafruit_GFX* gfx,
        int16_t x,
        int16_t y,
        uint16_t w,
        uint16_t h,
        uint16_t outline,
        uint16_t fill,
        uint16_t textcolor,
        char* label,
        uint8_t textsize
    ) {
        (void)gfx;
        (void)x;
        (void)y;
        (void)w;
        (void)h;
        (void)outline;
        (void)fill;
        (void)textcolor;
        (void)label;
        (void)textsize;
    }

    void drawButton(bool inverted = false) {
        (void)inverted;
    }

    bool contains(int16_t x, int16_t y) {
        (void)x;
        (void)y;
        return false;
    }

    void press(bool p) {
        currstate = p;
        laststate = currstate;
    }

    bool isPressed() {
        return currstate;
    }

    bool justPressed() {
        return currstate && !laststate;
    }

    bool justReleased() {
        return !currstate && laststate;
    }

private:
    bool currstate = false;
    bool laststate = false;
};