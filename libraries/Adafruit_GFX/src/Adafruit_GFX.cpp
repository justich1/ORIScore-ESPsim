#include <Adafruit_GFX.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

// Jednoduchý 5x7 font. Malá písmena se pro simulaci zobrazují jako velká.
// Každý byte reprezentuje jeden řádek, bity 4..0 jsou pixely zleva doprava.
const uint8_t* glyph5x7(unsigned char c) {
    static const uint8_t blank[7]    = {0,0,0,0,0,0,0};
    static const uint8_t unknown[7]  = {0x0E,0x11,0x01,0x06,0x04,0x00,0x04};
    static const uint8_t exclam[7]   = {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
    static const uint8_t quote[7]    = {0x0A,0x0A,0x0A,0,0,0,0};
    static const uint8_t hash[7]     = {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0};
    static const uint8_t percent[7]  = {0x19,0x1A,0x04,0x08,0x0B,0x13,0};
    static const uint8_t amp[7]      = {0x0C,0x12,0x14,0x08,0x15,0x12,0x0D};
    static const uint8_t apost[7]    = {0x04,0x04,0x08,0,0,0,0};
    static const uint8_t lparen[7]   = {0x02,0x04,0x08,0x08,0x08,0x04,0x02};
    static const uint8_t rparen[7]   = {0x08,0x04,0x02,0x02,0x02,0x04,0x08};
    static const uint8_t star[7]     = {0,0x04,0x15,0x0E,0x15,0x04,0};
    static const uint8_t plus[7]     = {0,0x04,0x04,0x1F,0x04,0x04,0};
    static const uint8_t comma[7]    = {0,0,0,0,0x0C,0x04,0x08};
    static const uint8_t dash[7]     = {0,0,0,0x1F,0,0,0};
    static const uint8_t dot[7]      = {0,0,0,0,0,0x0C,0x0C};
    static const uint8_t slash[7]    = {0x01,0x02,0x04,0x08,0x10,0,0};
    static const uint8_t colon[7]    = {0,0x0C,0x0C,0,0x0C,0x0C,0};
    static const uint8_t semicolon[7]= {0,0x0C,0x0C,0,0x0C,0x04,0x08};
    static const uint8_t less[7]     = {0x02,0x04,0x08,0x10,0x08,0x04,0x02};
    static const uint8_t equal[7]    = {0,0,0x1F,0,0x1F,0,0};
    static const uint8_t greater[7]  = {0x08,0x04,0x02,0x01,0x02,0x04,0x08};
    static const uint8_t at[7]       = {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E};
    static const uint8_t lbrack[7]   = {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E};
    static const uint8_t backslash[7]= {0x10,0x08,0x04,0x02,0x01,0,0};
    static const uint8_t rbrack[7]   = {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E};
    static const uint8_t caret[7]    = {0x04,0x0A,0x11,0,0,0,0};
    static const uint8_t underscore[7]={0,0,0,0,0,0,0x1F};
    static const uint8_t backtick[7] = {0x08,0x04,0x02,0,0,0,0};
    static const uint8_t lbrace[7]   = {0x02,0x04,0x04,0x08,0x04,0x04,0x02};
    static const uint8_t pipe[7]     = {0x04,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t rbrace[7]   = {0x08,0x04,0x04,0x02,0x04,0x04,0x08};
    static const uint8_t tilde[7]    = {0,0,0x09,0x16,0,0,0};
    static const uint8_t degree[7]   = {0x06,0x09,0x09,0x06,0,0,0};

    static const uint8_t digits[10][7] = {
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
        {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
        {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
        {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
        {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
        {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
        {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
        {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}
    };

    static const uint8_t letters[26][7] = {
        {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
        {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
        {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
        {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
        {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // G
        {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
        {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
        {0x07,0x02,0x02,0x02,0x12,0x12,0x0C}, // J
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
        {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
        {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
        {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
        {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
        {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
        {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
        {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
        {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
        {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
        {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, // W
        {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
        {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
        {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}  // Z
    };

    if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 'a' + 'A');
    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];

    switch (c) {
        case ' ': return blank;
        case '!': return exclam;
        case '"': return quote;
        case '#': return hash;
        case '%': return percent;
        case '&': return amp;
        case '\'': return apost;
        case '(': return lparen;
        case ')': return rparen;
        case '*': return star;
        case '+': return plus;
        case ',': return comma;
        case '-': return dash;
        case '.': return dot;
        case '/': return slash;
        case ':': return colon;
        case ';': return semicolon;
        case '<': return less;
        case '=': return equal;
        case '>': return greater;
        case '?': return unknown;
        case '@': return at;
        case '[': return lbrack;
        case '\\': return backslash;
        case ']': return rbrack;
        case '^': return caret;
        case '_': return underscore;
        case '`': return backtick;
        case '{': return lbrace;
        case '|': return pipe;
        case '}': return rbrace;
        case '~': return tilde;
        case 176: return degree;
        default: return unknown;
    }
}

inline int16_t clampRadius(int16_t r, int16_t w, int16_t h) {
    return std::max<int16_t>(0, std::min<int16_t>(r, std::min<int16_t>(w, h) / 2));
}

} // namespace

Adafruit_GFX::Adafruit_GFX(int16_t w, int16_t h)
    : WIDTH(w), HEIGHT(h), _width(w), _height(h), cursor_x(0), cursor_y(0),
      textcolor(WHITE), textbgcolor(BLACK), textsize_x(1), textsize_y(1),
      rotation(0), wrap(true) {}

void Adafruit_GFX::swapInt16(int16_t& a, int16_t& b) {
    int16_t t = a; a = b; b = t;
}

void Adafruit_GFX::writePixel(int16_t x, int16_t y, uint16_t color) { drawPixel(x, y, color); }
void Adafruit_GFX::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { fillRect(x, y, w, h, color); }
void Adafruit_GFX::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { drawFastVLine(x, y, h, color); }
void Adafruit_GFX::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { drawFastHLine(x, y, w, color); }

void Adafruit_GFX::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (h < 0) { y += h + 1; h = -h; }
    for (int16_t i = 0; i < h; ++i) drawPixel(x, y + i, color);
}

void Adafruit_GFX::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if (w < 0) { x += w + 1; w = -w; }
    for (int16_t i = 0; i < w; ++i) drawPixel(x + i, y, color);
}

void Adafruit_GFX::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) { swapInt16(x0, y0); swapInt16(x1, y1); }
    if (x0 > x1) { swapInt16(x0, x1); swapInt16(y0, y1); }
    int16_t dx = x1 - x0;
    int16_t dy = std::abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    for (; x0 <= x1; ++x0) {
        if (steep) drawPixel(y0, x0, color); else drawPixel(x0, y0, color);
        err -= dy;
        if (err < 0) { y0 += ystep; err += dx; }
    }
}

void Adafruit_GFX::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void Adafruit_GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    for (int16_t xx = x; xx < x + w; ++xx) drawFastVLine(xx, y, h, color);
}

void Adafruit_GFX::fillScreen(uint16_t color) { fillRect(0, 0, _width, _height, color); }

void Adafruit_GFX::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r < 0) return;
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    drawPixel(x0, y0 + r, color); drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color); drawPixel(x0 - r, y0, color);
    while (x < y) {
        if (f >= 0) { --y; ddF_y += 2; f += ddF_y; }
        ++x; ddF_x += 2; f += ddF_x;
        drawPixel(x0 + x, y0 + y, color); drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color); drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color); drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color); drawPixel(x0 - y, y0 - x, color);
    }
}

void Adafruit_GFX::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r < 0) return;
    for (int16_t y = -r; y <= r; ++y) {
        int16_t x = static_cast<int16_t>(std::sqrt(static_cast<double>(r) * r - static_cast<double>(y) * y));
        drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
    }
}

void Adafruit_GFX::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void Adafruit_GFX::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color) {
    if (y0 > y1) { swapInt16(y0, y1); swapInt16(x0, x1); }
    if (y1 > y2) { swapInt16(y2, y1); swapInt16(x2, x1); }
    if (y0 > y1) { swapInt16(y0, y1); swapInt16(x0, x1); }

    if (y0 == y2) {
        int16_t a = std::min<int16_t>(x0, std::min<int16_t>(x1, x2));
        int16_t b = std::max<int16_t>(x0, std::max<int16_t>(x1, x2));
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int32_t dx01 = x1 - x0, dy01 = y1 - y0;
    int32_t dx02 = x2 - x0, dy02 = y2 - y0;
    int32_t dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    int16_t y, last = (y1 == y2) ? y1 : y1 - 1;

    for (y = y0; y <= last; ++y) {
        int16_t a = x0 + static_cast<int16_t>(dy01 ? sa / dy01 : 0);
        int16_t b = x0 + static_cast<int16_t>(dy02 ? sb / dy02 : 0);
        sa += dx01; sb += dx02;
        if (a > b) swapInt16(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; ++y) {
        int16_t a = x1 + static_cast<int16_t>(dy12 ? sa / dy12 : 0);
        int16_t b = x0 + static_cast<int16_t>(dy02 ? sb / dy02 : 0);
        sa += dx12; sb += dx02;
        if (a > b) swapInt16(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
}

void Adafruit_GFX::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    r = clampRadius(r, w, h);
    drawFastHLine(x + r, y, w - 2 * r, color);
    drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
    drawFastVLine(x, y + r, h - 2 * r, color);
    drawFastVLine(x + w - 1, y + r, h - 2 * r, color);
    for (int16_t yy = 0; yy <= r; ++yy) {
        int16_t xx = static_cast<int16_t>(std::sqrt(static_cast<double>(r) * r - static_cast<double>(yy) * yy));
        drawPixel(x + r - xx, y + r - yy, color);
        drawPixel(x + w - r - 1 + xx, y + r - yy, color);
        drawPixel(x + r - xx, y + h - r - 1 + yy, color);
        drawPixel(x + w - r - 1 + xx, y + h - r - 1 + yy, color);
    }
}

void Adafruit_GFX::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    r = clampRadius(r, w, h);
    fillRect(x + r, y, w - 2 * r, h, color);
    for (int16_t yy = -r; yy <= r; ++yy) {
        int16_t xx = static_cast<int16_t>(std::sqrt(static_cast<double>(r) * r - static_cast<double>(yy) * yy));
        drawFastHLine(x + r - xx, y + r + yy, xx, color);
        drawFastHLine(x + w - r, y + r + yy, xx, color);
        drawFastHLine(x + r - xx, y + h - r - 1 + yy, xx, color);
        drawFastHLine(x + w - r, y + h - r - 1 + yy, xx, color);
    }
}

void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color) {
    if (!bitmap || w <= 0 || h <= 0) return;
    int16_t byteWidth = (w + 7) / 8;
    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            uint8_t b = bitmap[j * byteWidth + i / 8];
            if (b & (0x80 >> (i & 7))) drawPixel(x + i, y + j, color);
        }
    }
}

void Adafruit_GFX::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, uint16_t color, uint16_t bg) {
    if (!bitmap || w <= 0 || h <= 0) return;
    int16_t byteWidth = (w + 7) / 8;
    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            uint8_t b = bitmap[j * byteWidth + i / 8];
            drawPixel(x + i, y + j, (b & (0x80 >> (i & 7))) ? color : bg);
        }
    }
}

void Adafruit_GFX::drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                               int16_t w, int16_t h, uint16_t color) {
    if (!bitmap || w <= 0 || h <= 0) return;
    int16_t byteWidth = (w + 7) / 8;
    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            uint8_t b = bitmap[j * byteWidth + i / 8];
            if (b & (1 << (i & 7))) drawPixel(x + i, y + j, color);
        }
    }
}

void Adafruit_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
                            uint16_t color, uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
}

void Adafruit_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
                            uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {
    if (size_x == 0) size_x = 1;
    if (size_y == 0) size_y = 1;
    const uint8_t* rows = glyph5x7(c);

    for (int8_t row = 0; row < 8; ++row) {
        for (int8_t col = 0; col < 6; ++col) {
            bool on = row < 7 && col < 5 && (rows[row] & (1 << (4 - col)));
            if (on || bg != color) {
                uint16_t pxColor = on ? color : bg;
                if (size_x == 1 && size_y == 1) drawPixel(x + col, y + row, pxColor);
                else fillRect(x + col * size_x, y + row * size_y, size_x, size_y, pxColor);
            }
        }
    }
}

void Adafruit_GFX::setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }
int16_t Adafruit_GFX::getCursorX() const { return cursor_x; }
int16_t Adafruit_GFX::getCursorY() const { return cursor_y; }
void Adafruit_GFX::setTextColor(uint16_t c) { textcolor = c; textbgcolor = c; }
void Adafruit_GFX::setTextColor(uint16_t c, uint16_t bg) { textcolor = c; textbgcolor = bg; }
void Adafruit_GFX::setTextSize(uint8_t s) { if (s == 0) s = 1; textsize_x = s; textsize_y = s; }
void Adafruit_GFX::setTextSize(uint8_t sx, uint8_t sy) { textsize_x = sx ? sx : 1; textsize_y = sy ? sy : 1; }
void Adafruit_GFX::setTextWrap(bool w) { wrap = w; }

void Adafruit_GFX::setRotation(uint8_t r) {
    rotation = r & 3;
    if (rotation & 1) { _width = HEIGHT; _height = WIDTH; }
    else { _width = WIDTH; _height = HEIGHT; }
}
uint8_t Adafruit_GFX::getRotation() const { return rotation; }
int16_t Adafruit_GFX::width() const { return _width; }
int16_t Adafruit_GFX::height() const { return _height; }

size_t Adafruit_GFX::write(uint8_t c) {
    if (c == '\r') return 1;
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += static_cast<int16_t>(textsize_y) * 8;
        return 1;
    }

    int16_t charWidth = static_cast<int16_t>(textsize_x) * 6;
    if (wrap && cursor_x + charWidth > _width) {
        cursor_x = 0;
        cursor_y += static_cast<int16_t>(textsize_y) * 8;
    }

    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
    cursor_x += charWidth;
    return 1;
}

void Adafruit_GFX::getTextBounds(const char* string, int16_t x, int16_t y,
                                 int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    if (!string || !*string) {
        if (x1) *x1 = x; if (y1) *y1 = y; if (w) *w = 0; if (h) *h = 0;
        return;
    }

    int16_t cx = x, cy = y;
    int16_t minX = x, minY = y, maxX = x, maxY = y;
    bool any = false;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(string); *p; ++p) {
        if (*p == '\r') continue;
        if (*p == '\n') { cx = 0; cy += static_cast<int16_t>(textsize_y) * 8; continue; }
        int16_t cw = static_cast<int16_t>(textsize_x) * 6;
        int16_t ch = static_cast<int16_t>(textsize_y) * 8;
        if (wrap && cx + cw > _width) { cx = 0; cy += ch; }
        if (!any) { minX = cx; minY = cy; any = true; }
        maxX = std::max<int16_t>(maxX, cx + cw - 1);
        maxY = std::max<int16_t>(maxY, cy + ch - 1);
        cx += cw;
    }
    if (x1) *x1 = any ? minX : x;
    if (y1) *y1 = any ? minY : y;
    if (w) *w = any ? static_cast<uint16_t>(maxX - minX + 1) : 0;
    if (h) *h = any ? static_cast<uint16_t>(maxY - minY + 1) : 0;
}

void Adafruit_GFX::getTextBounds(const String& string, int16_t x, int16_t y,
                                 int16_t* x1, int16_t* y1, uint16_t* w, uint16_t* h) {
    getTextBounds(string.c_str(), x, y, x1, y1, w, h);
}

void Adafruit_GFX_Button::initButton(Adafruit_GFX* gfx, int16_t x, int16_t y,
                                     uint16_t w, uint16_t h, uint16_t outline,
                                     uint16_t fill, uint16_t textcolor,
                                     char* label, uint8_t textsize) {
    initButtonUL(gfx, x - static_cast<int16_t>(w / 2), y - static_cast<int16_t>(h / 2),
                 w, h, outline, fill, textcolor, label, textsize);
}

void Adafruit_GFX_Button::initButtonUL(Adafruit_GFX* gfx, int16_t x1, int16_t y1,
                                       uint16_t w, uint16_t h, uint16_t outline,
                                       uint16_t fill, uint16_t textcolor,
                                       char* label, uint8_t textsize) {
    _gfx = gfx; _x1 = x1; _y1 = y1; _w = w; _h = h;
    _outlinecolor = outline; _fillcolor = fill; _textcolor = textcolor;
    _textsize = textsize ? textsize : 1;
    std::strncpy(_label, label ? label : "", sizeof(_label) - 1);
    _label[sizeof(_label) - 1] = '\0';
}

void Adafruit_GFX_Button::drawButton(bool inverted) {
    if (!_gfx) return;
    uint16_t fill = inverted ? _textcolor : _fillcolor;
    uint16_t text = inverted ? _fillcolor : _textcolor;
    _gfx->fillRoundRect(_x1, _y1, _w, _h, std::min<uint16_t>(_w, _h) / 4, fill);
    _gfx->drawRoundRect(_x1, _y1, _w, _h, std::min<uint16_t>(_w, _h) / 4, _outlinecolor);
    int16_t textW = static_cast<int16_t>(std::strlen(_label)) * 6 * _textsize;
    int16_t textH = 8 * _textsize;
    _gfx->setCursor(_x1 + (static_cast<int16_t>(_w) - textW) / 2,
                    _y1 + (static_cast<int16_t>(_h) - textH) / 2);
    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);
    _gfx->print(_label);
}

bool Adafruit_GFX_Button::contains(int16_t x, int16_t y) {
    return x >= _x1 && x < _x1 + static_cast<int16_t>(_w) &&
           y >= _y1 && y < _y1 + static_cast<int16_t>(_h);
}
void Adafruit_GFX_Button::press(bool p) { laststate = currstate; currstate = p; }
bool Adafruit_GFX_Button::isPressed() { return currstate; }
bool Adafruit_GFX_Button::justPressed() { return currstate && !laststate; }
bool Adafruit_GFX_Button::justReleased() { return !currstate && laststate; }
