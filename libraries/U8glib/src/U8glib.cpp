#include "U8glib.h"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

const uint8_t u8g_font_6x10[] = {0};
const uint8_t u8g_font_5x7[] = {0};
const uint8_t u8g_font_unifont[] = {0};

static const uint8_t* glyph5x7(char c) {
  static const uint8_t blank[7] = {0,0,0,0,0,0,0};
  static const uint8_t unknown[7] = {0x1e,0x21,0x01,0x0e,0x04,0x00,0x04};
  static const uint8_t space[7] = {0,0,0,0,0,0,0};
  static const uint8_t colon[7] = {0,0x04,0x04,0,0x04,0x04,0};
  static const uint8_t dot[7] = {0,0,0,0,0,0x0c,0x0c};
  static const uint8_t dash[7] = {0,0,0,0x1f,0,0,0};
  static const uint8_t slash[7] = {0x01,0x02,0x04,0x08,0x10,0,0};
  static const uint8_t zero[7] = {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e};
  static const uint8_t one[7] = {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e};
  static const uint8_t two[7] = {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f};
  static const uint8_t three[7] = {0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e};
  static const uint8_t four[7] = {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02};
  static const uint8_t five[7] = {0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e};
  static const uint8_t six[7] = {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e};
  static const uint8_t seven[7] = {0x1f,0x01,0x02,0x04,0x08,0x08,0x08};
  static const uint8_t eight[7] = {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e};
  static const uint8_t nine[7] = {0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e};
  static const uint8_t A[7] = {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11};
  static const uint8_t B[7] = {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e};
  static const uint8_t C[7] = {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e};
  static const uint8_t D[7] = {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e};
  static const uint8_t E[7] = {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f};
  static const uint8_t F[7] = {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10};
  static const uint8_t G[7] = {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f};
  static const uint8_t H[7] = {0x11,0x11,0x11,0x1f,0x11,0x11,0x11};
  static const uint8_t I[7] = {0x0e,0x04,0x04,0x04,0x04,0x04,0x0e};
  static const uint8_t J[7] = {0x07,0x02,0x02,0x02,0x12,0x12,0x0c};
  static const uint8_t K[7] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
  static const uint8_t L[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1f};
  static const uint8_t M[7] = {0x11,0x1b,0x15,0x15,0x11,0x11,0x11};
  static const uint8_t N[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
  static const uint8_t O[7] = {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e};
  static const uint8_t P[7] = {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10};
  static const uint8_t Q[7] = {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d};
  static const uint8_t R[7] = {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11};
  static const uint8_t S[7] = {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e};
  static const uint8_t T[7] = {0x1f,0x04,0x04,0x04,0x04,0x04,0x04};
  static const uint8_t U[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0e};
  static const uint8_t V[7] = {0x11,0x11,0x11,0x11,0x11,0x0a,0x04};
  static const uint8_t W[7] = {0x11,0x11,0x11,0x15,0x15,0x15,0x0a};
  static const uint8_t X[7] = {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11};
  static const uint8_t Y[7] = {0x11,0x11,0x0a,0x04,0x04,0x04,0x04};
  static const uint8_t Z[7] = {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f};

  if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
  switch (c) {
    case ' ': return space;
    case ':': return colon;
    case '.': return dot;
    case '-': return dash;
    case '/': return slash;
    case '0': return zero; case '1': return one; case '2': return two; case '3': return three; case '4': return four;
    case '5': return five; case '6': return six; case '7': return seven; case '8': return eight; case '9': return nine;
    case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D; case 'E': return E; case 'F': return F;
    case 'G': return G; case 'H': return H; case 'I': return I; case 'J': return J; case 'K': return K; case 'L': return L;
    case 'M': return M; case 'N': return N; case 'O': return O; case 'P': return P; case 'Q': return Q; case 'R': return R;
    case 'S': return S; case 'T': return T; case 'U': return U; case 'V': return V; case 'W': return W; case 'X': return X;
    case 'Y': return Y; case 'Z': return Z;
  }
  return unknown;
}

U8GLIB::U8GLIB(int width, int height, const char* controller, const char* iface)
  : _width(width), _height(height), _controller(controller ? controller : "DISPLAY"), _iface(iface ? iface : "SIM") {
  _buffer.assign(_width * ((_height + 7) / 8), 0);
  emitConfig();
}

void U8GLIB::begin() { emitConfig(); }
bool U8GLIB::firstPage() { clear(); _pageActive = true; return true; }
bool U8GLIB::nextPage() { if (_pageActive) { emitFrame(); _pageActive = false; } return false; }
void U8GLIB::setFont(const uint8_t*) {}
void U8GLIB::setColorIndex(uint8_t color) { _color = color ? 1 : 0; }
void U8GLIB::setPrintPos(int x, int y) { _printX = x; _printY = y; }
void U8GLIB::sleepOn() { _sleep = true; }
void U8GLIB::sleepOff() { _sleep = false; }
void U8GLIB::setRot180() {}
void U8GLIB::undoRotation() {}
int U8GLIB::getWidth() const { return _width; }
int U8GLIB::getHeight() const { return _height; }

void U8GLIB::clear() { std::fill(_buffer.begin(), _buffer.end(), 0); }

void U8GLIB::putPixel(int x, int y, bool on) {
  if (x < 0 || y < 0 || x >= _width || y >= _height) return;
  int index = (y / 8) * _width + x;
  uint8_t mask = (uint8_t)(1u << (y & 7));
  if (on) _buffer[index] |= mask;
  else _buffer[index] &= (uint8_t)~mask;
}

void U8GLIB::drawPixel(int x, int y) { putPixel(x, y, _color != 0); }

void U8GLIB::drawLine(int x0, int y0, int x1, int y1) {
  int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    drawPixel(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void U8GLIB::drawHLine(int x, int y, int w) { for (int i = 0; i < w; i++) drawPixel(x + i, y); }
void U8GLIB::drawVLine(int x, int y, int h) { for (int i = 0; i < h; i++) drawPixel(x, y + i); }

void U8GLIB::drawBox(int x, int y, int w, int h) {
  for (int yy = 0; yy < h; yy++) for (int xx = 0; xx < w; xx++) drawPixel(x + xx, y + yy);
}

void U8GLIB::drawFrame(int x, int y, int w, int h) {
  drawHLine(x, y, w);
  drawHLine(x, y + h - 1, w);
  drawVLine(x, y, h);
  drawVLine(x + w - 1, y, h);
}

void U8GLIB::drawChar5x7(int x, int y, char c) {
  const uint8_t* rows = glyph5x7(c);
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      if (rows[row] & (1 << (4 - col))) putPixel(x + col, y + row, _color != 0);
      else if (_color == 0) putPixel(x + col, y + row, false);
    }
  }
}

void U8GLIB::drawStr(int x, int y, const char* s) {
  if (!s) return;
  int cx = x;
  int top = y - 7;
  for (const char* p = s; *p; ++p) {
    drawChar5x7(cx, top, *p);
    cx += 6;
    if (cx >= _width) break;
  }
}

void U8GLIB::drawStr(int x, int y, const String& s) { drawStr(x, y, s.c_str()); }

size_t U8GLIB::print(const char* s) {
  if (!s) return 0;
  drawStr(_printX, _printY, s);
  size_t n = std::string(s).size();
  _printX += (int)n * 6;
  return n;
}
size_t U8GLIB::print(const String& s) { return print(s.c_str()); }
size_t U8GLIB::print(char c) { char b[2] = {c, 0}; return print(b); }
size_t U8GLIB::print(int n) { return print(String(n)); }
size_t U8GLIB::print(unsigned int n) { return print(String(n)); }
size_t U8GLIB::print(long n) { return print(String(n)); }
size_t U8GLIB::print(unsigned long n) { return print(String(n)); }
size_t U8GLIB::println(const char* s) { size_t n = print(s); _printX = 0; _printY += 10; return n; }
size_t U8GLIB::println(const String& s) { return println(s.c_str()); }

void U8GLIB::emitConfig() {
  std::cout << "DISPLAYCFG TYPE=" << _controller << " IF=" << _iface << " W=" << _width << " H=" << _height << std::endl;
}

void U8GLIB::emitFrame() {
  if (_sleep) return;
  std::ostringstream oss;
  oss << "DISPLAYFRAME W=" << _width << " H=" << _height << " HEX=";
  oss << std::hex << std::setfill('0');
  for (uint8_t b : _buffer) oss << std::setw(2) << (int)b;
  std::cout << oss.str() << std::endl;
}

U8GLIB_SSD1306_128X64::U8GLIB_SSD1306_128X64(uint8_t) : U8GLIB(128, 64, "SSD1306", "I2C") {}
U8GLIB_SSD1306_128X64::U8GLIB_SSD1306_128X64(uint8_t, uint8_t, uint8_t) : U8GLIB(128, 64, "SSD1306", "SPI") {}
U8GLIB_SSD1306_128X64::U8GLIB_SSD1306_128X64(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) : U8GLIB(128, 64, "SSD1306", "SPI") {}

U8GLIB_SSD1306_128X32::U8GLIB_SSD1306_128X32(uint8_t) : U8GLIB(128, 32, "SSD1306", "I2C") {}
U8GLIB_SSD1306_128X32::U8GLIB_SSD1306_128X32(uint8_t, uint8_t, uint8_t) : U8GLIB(128, 32, "SSD1306", "SPI") {}
U8GLIB_SSD1306_128X32::U8GLIB_SSD1306_128X32(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) : U8GLIB(128, 32, "SSD1306", "SPI") {}

void simDisplayConfigure(const String& controller, const String& iface, int width, int height) {
  std::cout << "DISPLAYCFG TYPE=" << controller << " IF=" << iface << " W=" << width << " H=" << height << std::endl;
}
