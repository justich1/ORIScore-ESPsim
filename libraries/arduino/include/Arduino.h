#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <deque>
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <type_traits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <cstdarg>
#include <ctime>
#include <cctype>
#include <algorithm>
#include <ctime>

#ifndef ADC_0db
#define ADC_0db 0
#endif

#ifndef ADC_2_5db
#define ADC_2_5db 1
#endif

#ifndef ADC_6db
#define ADC_6db 2
#endif

#ifndef ADC_11db
#define ADC_11db 3
#endif

inline void analogReadResolution(int bits) {
    (void)bits;
}

inline void analogSetAttenuation(int attenuation) {
    (void)attenuation;
}

using byte = uint8_t;
using boolean = bool;
using word = uint16_t;

class __FlashStringHelper;

#define HIGH 1

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define LOW 0

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define INPUT 0

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define OUTPUT 1

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#define CHANGE 1

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define FALLING 2

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define RISING 3

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#define DEC 10

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define HEX 16

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define OCT 8

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define BIN 2

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#ifndef HALF_PI
#define HALF_PI 1.5707963267948966192313216916398
#endif
#ifndef TWO_PI
#define TWO_PI 6.283185307179586476925286766559
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769236907684886
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.295779513082320876798154814105
#endif
#ifndef radians
#define radians(deg) ((deg) * DEG_TO_RAD)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * RAD_TO_DEG)
#endif
#ifndef sq
#define sq(x) ((x) * (x))
#endif
#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#define D0 16

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D1 5

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D2 4

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D3 0

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D4 2

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D5 14

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D6 12

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D7 13

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define D8 15
#define A0 0

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#define PROGMEM

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define ICACHE_FLASH_ATTR

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define F(x) x

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define PSTR(x) x

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#ifndef NAN
#define NAN std::nanf("")

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#endif

class String {
public:
  std::string v;

  String();
  String(const char* s);
  String(const std::string& s);
  String(char c);
  String(int n);
  String(unsigned int n);
  String(long n);
  String(unsigned long n);
  String(long long n);
  String(unsigned long long n);
  String(int n, int base);
  String(unsigned int n, int base);
  String(long n, int base);
  String(unsigned long n, int base);
  String(long long n, int base);
  String(unsigned long long n, int base);
  String(float f, int decimals = 2);
  String(double f, int decimals = 2);

  size_t length() const;
  bool isEmpty() const;
  bool reserve(size_t n);
  size_t capacity() const;
  const char* c_str() const;
  void toCharArray(char* buffer, size_t size) const;
  bool concat(const String& s);
  bool concat(const char* s);
  bool concat(char c);
  bool concat(int n);
  bool concat(unsigned int n);
  bool concat(long n);
  bool concat(unsigned long n);
  int compareTo(const String& s) const;
  bool equals(const String& s) const;
  bool equals(const char* s) const;
  bool equalsIgnoreCase(const String& s) const;

  int indexOf(const String& needle) const;
  int indexOf(const String& needle, size_t from) const;
  int indexOf(const char* needle) const;
  int indexOf(const char* needle, size_t from) const;
  int indexOf(char needle) const;
  int indexOf(char needle, size_t from) const;
  int lastIndexOf(const String& needle) const;
  bool startsWith(const String& prefix) const;
  bool endsWith(const String& suffix) const;

  String substring(size_t from) const;
  String substring(size_t from, size_t to) const;
  void remove(size_t index);
  void remove(size_t index, size_t count);
  void replace(const String& from, const String& to);
  void trim();
  void toUpperCase();
  void toLowerCase();

  int toInt() const;
  float toFloat() const;

  char charAt(size_t idx) const;
  void setCharAt(size_t idx, char c);
  char operator[](size_t idx) const;
  char& operator[](size_t idx);

  String& operator+=(const String& other);
  String& operator+=(const char* other);
  String& operator+=(char c);
  String& operator+=(int n);
  String& operator+=(unsigned int n);
  String& operator+=(long n);
  String& operator+=(unsigned long n);
  String& operator+=(float f);
  String& operator+=(double f);

  explicit operator bool() const;
  operator std::string() const;
};

String operator+(const String& a, const String& b);
String operator+(const String& a, const char* b);
String operator+(const char* a, const String& b);
String operator+(const String& a, char b);

bool operator==(const String& a, const String& b);
bool operator==(const String& a, const char* b);
bool operator==(const char* a, const String& b);
bool operator!=(const String& a, const String& b);
bool operator!=(const String& a, const char* b);
bool operator!=(const char* a, const String& b);
bool operator<(const String& a, const String& b);
bool operator>(const String& a, const String& b);
bool operator<=(const String& a, const String& b);
bool operator>=(const String& a, const String& b);

std::ostream& operator<<(std::ostream& os, const String& s);


class Print;

class Printable {
public:
  virtual ~Printable() = default;
  virtual size_t printTo(Print& p) const = 0;
};

class Print {
private:
  int write_error = 0;
  size_t printNumber(unsigned long long n, uint8_t base);
  size_t printFloat(double number, uint8_t digits);

protected:
  void setWriteError(int err = 1) { write_error = err; }

public:
  Print() = default;
  virtual ~Print() = default;

  int getWriteError() const { return write_error; }
  void clearWriteError() { setWriteError(0); }

  virtual size_t write(uint8_t b) = 0;
  virtual size_t write(const uint8_t* buffer, size_t size);
  size_t write(const char* str);
  size_t write(const char* buffer, size_t size);
  size_t write(const String& s);

  virtual int availableForWrite() { return 0; }

  size_t print(const __FlashStringHelper* s);
  size_t print(const String& s);
  size_t print(const char s[]);
  size_t print(char c);
  size_t print(unsigned char b, int base = DEC);
  size_t print(int n, int base = DEC);
  size_t print(unsigned int n, int base = DEC);
  size_t print(long n, int base = DEC);
  size_t print(unsigned long n, int base = DEC);
  size_t print(long long n, int base = DEC);
  size_t print(unsigned long long n, int base = DEC);
  size_t print(float f, int digits = 2);
  size_t print(double f, int digits = 2);
  size_t print(const Printable& x);

  size_t println(const __FlashStringHelper* s);
  size_t println(const String& s);
  size_t println(const char s[]);
  size_t println(char c);
  size_t println(unsigned char b, int base = DEC);
  size_t println(int n, int base = DEC);
  size_t println(unsigned int n, int base = DEC);
  size_t println(long n, int base = DEC);
  size_t println(unsigned long n, int base = DEC);
  size_t println(long long n, int base = DEC);
  size_t println(unsigned long long n, int base = DEC);
  size_t println(float f, int digits = 2);
  size_t println(double f, int digits = 2);
  size_t println(const Printable& x);
  size_t println();

  virtual void flush() {}
};

enum LookaheadMode {
  SKIP_ALL,
  SKIP_NONE,
  SKIP_WHITESPACE
};

class Stream : public Print {
protected:
  unsigned long _timeout = 1000;
  unsigned long _startMillis = 0;
  int timedRead();
  int timedPeek();
  int peekNextDigit(LookaheadMode lookahead, bool detectDecimal);

public:
  Stream() = default;
  virtual ~Stream() = default;

  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;

  void setTimeout(unsigned long timeout);
  unsigned long getTimeout() const { return _timeout; }

  bool find(const char* target);
  bool find(char* target) { return find((const char*)target); }
  bool find(uint8_t* target) { return find((const char*)target); }
  bool find(const char* target, size_t length);
  bool find(char* target, size_t length) { return find((const char*)target, length); }
  bool find(uint8_t* target, size_t length) { return find((const char*)target, length); }
  bool find(char target) { return find(&target, 1); }

  bool findUntil(const char* target, const char* terminator);
  bool findUntil(char* target, char* terminator) { return findUntil((const char*)target, (const char*)terminator); }
  bool findUntil(uint8_t* target, char* terminator) { return findUntil((const char*)target, (const char*)terminator); }
  bool findUntil(const char* target, size_t targetLen, const char* terminate, size_t termLen);
  bool findUntil(char* target, size_t targetLen, char* terminate, size_t termLen) {
    return findUntil((const char*)target, targetLen, (const char*)terminate, termLen);
  }

  long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = '\x01');
  float parseFloat(LookaheadMode lookahead = SKIP_ALL, char ignore = '\x01');
  size_t readBytes(char* buffer, size_t length);
  size_t readBytes(uint8_t* buffer, size_t length) { return readBytes((char*)buffer, length); }
  size_t readBytesUntil(char terminator, char* buffer, size_t length);
  size_t readBytesUntil(char terminator, uint8_t* buffer, size_t length) { return readBytesUntil(terminator, (char*)buffer, length); }
  String readString();
  String readStringUntil(char terminator);

protected:
  long parseInt(char ignore) { return parseInt(SKIP_ALL, ignore); }
  float parseFloat(char ignore) { return parseFloat(SKIP_ALL, ignore); }
};

class IPAddress : public Printable {
private:
  union {
    uint8_t bytes[4];
    uint32_t dword;
  } _address;

  uint8_t* raw_address() { return _address.bytes; }

public:
  IPAddress();
  IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth);
  explicit IPAddress(uint32_t address);
  IPAddress(const uint8_t* address);

  bool fromString(const char* address);
  bool fromString(const String& address) { return fromString(address.c_str()); }

  operator uint32_t() const { return _address.dword; }
  operator String() const { return toString(); }
  bool operator==(const IPAddress& addr) const { return _address.dword == addr._address.dword; }
  bool operator!=(const IPAddress& addr) const { return !(*this == addr); }
  bool operator==(const uint8_t* addr) const;

  uint8_t operator[](int index) const { return _address.bytes[index]; }
  uint8_t& operator[](int index) { return _address.bytes[index]; }

  IPAddress& operator=(const uint8_t* address);
  IPAddress& operator=(uint32_t address);

  String toString() const;
  size_t printTo(Print& p) const override;

  friend class Client;
};

// Windows / WinSock definuje INADDR_NONE jako makro.
// To by rozbilo deklaraci Arduino konstanty stejného jména.
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif
extern const IPAddress INADDR_NONE;

class Client : public Stream {
public:
  virtual int connect(IPAddress ip, uint16_t port) = 0;
  virtual int connect(const char* host, uint16_t port) = 0;
  virtual size_t write(uint8_t) override = 0;
  virtual size_t write(const uint8_t* buf, size_t size) override = 0;
  virtual int available() override = 0;
  virtual int read() override = 0;
  virtual int read(uint8_t* buf, size_t size) = 0;
  virtual int peek() override = 0;
  virtual void flush() override = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
  virtual operator bool() = 0;

protected:
  uint8_t* rawIPAddress(IPAddress& addr) { return addr.raw_address(); }
};

class Server : public Print {
public:
  virtual void begin() = 0;
};

class UDP : public Stream {
public:
  virtual uint8_t begin(uint16_t) = 0;
  virtual uint8_t beginMulticast(IPAddress, uint16_t) { return 0; }
  virtual void stop() = 0;
  virtual int beginPacket(IPAddress ip, uint16_t port) = 0;
  virtual int beginPacket(const char* host, uint16_t port) = 0;
  virtual int endPacket() = 0;
  virtual size_t write(uint8_t) override = 0;
  virtual size_t write(const uint8_t* buffer, size_t size) override = 0;
  virtual int parsePacket() = 0;
  virtual int available() override = 0;
  virtual int read() override = 0;
  virtual int read(unsigned char* buffer, size_t len) = 0;
  virtual int read(char* buffer, size_t len) = 0;
  virtual int peek() override = 0;
  virtual void flush() override = 0;
  virtual IPAddress remoteIP() = 0;
  virtual uint16_t remotePort() = 0;
};


unsigned long millis();
void delay(unsigned long ms);
void yield();
void simSetMillis(unsigned long ms);
void simAdvanceMillis(unsigned long ms);
double simGetTimeScale();
void simSetTimeScale(double scale);
unsigned long simConsumeManualAdvanceMillis();

struct SimVirtualDevice {
  String type;
  String name;
  String pin;
  String address;
  String value;
  String humidity;
  String pressure;
  bool connected;
};

void simVirtualHwClear();
void simVirtualHwSet(const String& type, const String& name, const String& pin, const String& address,
                     const String& value, const String& humidity, const String& pressure, bool connected);
int simVirtualHwCount(const String& type);
bool simVirtualHwGetByTypeIndex(const String& type, int index, SimVirtualDevice* out);
bool simVirtualHwFindDs18b20ByAddress(const String& address, SimVirtualDevice* out);
bool simVirtualHwFindDhtByPin(uint8_t pin, SimVirtualDevice* out);
int simVirtualHwAnalogRaw(uint8_t pin);

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
void simSetDigitalInput(uint8_t pin, uint8_t value);
void simSetAnalogInput(uint8_t pin, int value);
void simClearAnalogInputs();
int simGetDigitalOutput(uint8_t pin);
int simGetPinMode(uint8_t pin);
int simGetDigitalValue(uint8_t pin);

int analogRead(uint8_t pin);
void analogWrite(uint8_t pin, int value);
void attachInterrupt(uint8_t pin, void (*callback)(), int mode);
void detachInterrupt(uint8_t pin);
void noInterrupts();
void interrupts();
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout = 1000000UL);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder);
long map(long x, long in_min, long in_max, long out_min, long out_max);

long random(long max);
long random(long min, long max);
void randomSeed(unsigned long seed);

void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1);
void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1, const char* server2);
bool getLocalTime(struct tm* info, uint32_t ms = 5000);

uint8_t highByte(uint16_t w);
uint8_t lowByte(uint16_t w);

class FakeSerial : public Stream {
public:
  using Print::write;

  FakeSerial();
  explicit FakeSerial(int uartNo);
  virtual ~FakeSerial();

  void begin(uint32_t baud);
    void begin(uint32_t baud, uint32_t config, int8_t rxPin, int8_t txPin);
  void end();
  int available() override;
  int read() override;
  int peek() override;
  void flush() override;

  size_t write(uint8_t b) override;
  size_t write(const uint8_t* buffer, size_t size) override;

  size_t print(const __FlashStringHelper* s);
  size_t print(const String& s);
  size_t print(const char* s);
  size_t print(char c);
  size_t print(unsigned char n);
  size_t print(int n);
  size_t print(unsigned int n);
  size_t print(long n);
  size_t print(unsigned long n);
  size_t print(long long n);
  size_t print(unsigned long long n);
  size_t print(float f);
  size_t print(double f);
  size_t print(const Printable& x);

  size_t print(unsigned char n, int base);
  size_t print(int n, int base);
  size_t print(unsigned int n, int base);
  size_t print(long n, int base);
  size_t print(unsigned long n, int base);
  size_t print(long long n, int base);
  size_t print(unsigned long long n, int base);
  size_t print(float f, int digits);
  size_t print(double f, int digits);

  size_t println();
  size_t println(const __FlashStringHelper* s);
  size_t println(const String& s);
  size_t println(const char* s);
  size_t println(char c);
  size_t println(unsigned char n);
  size_t println(int n);
  size_t println(unsigned int n);
  size_t println(long n);
  size_t println(unsigned long n);
  size_t println(long long n);
  size_t println(unsigned long long n);
  size_t println(float f);
  size_t println(double f);
  size_t println(const Printable& x);

  size_t println(unsigned char n, int base);
  size_t println(int n, int base);
  size_t println(unsigned int n, int base);
  size_t println(long n, int base);
  size_t println(unsigned long n, int base);
  size_t println(long long n, int base);
  size_t println(unsigned long long n, int base);
  size_t println(float f, int digits);
  size_t println(double f, int digits);
  size_t println(const struct tm* timeinfo, const char* format);
  size_t printf(const char* fmt, ...);

  void injectRx(const String& s);
  String takeTxLog();
  String getTxLog() const;
  void clearTxLog();

private:
  mutable std::recursive_mutex serialMutex;
  std::deque<char> rx;
  std::string tx;
  uint32_t baudRate = 0;
};

extern FakeSerial Serial;
extern FakeSerial Serial1;
extern FakeSerial Serial2;
extern FakeSerial Serial3;
using HardwareSerial = FakeSerial;

void simInjectSerialRxAll(const String& s);
int simGetSerialPortCount();
String simTakeSerialTxByIndex(int index);
void simClearSerialTxAll();


class FakeESPClass {
public:
  void restart();
  uint32_t getFreeHeap();
  uint32_t getMaxFreeBlockSize();
  uint32_t getFreeSketchSpace();
  uint32_t getSketchSize();
  uint32_t getFlashChipSize();
  uint64_t getEfuseMac();
  String getResetReason();
  void wdtFeed();
  void resetHeap(uint32_t freeHeap = 50000, uint32_t maxBlock = 32000);

private:
  uint32_t fakeFreeHeap = 50000;
  uint32_t fakeMaxBlock = 32000;
};

extern FakeESPClass ESP;

template <typename A, typename B>
auto min(A a, B b) -> std::common_type_t<A, B> {
  return (a < b) ? static_cast<std::common_type_t<A, B>>(a) : static_cast<std::common_type_t<A, B>>(b);
}

template <typename A, typename B>
auto max(A a, B b) -> std::common_type_t<A, B> {
  return (a > b) ? static_cast<std::common_type_t<A, B>>(a) : static_cast<std::common_type_t<A, B>>(b);
}
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define bitSet(value, bit) ((value) |= (1UL << (bit)))

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif
#define bit(b) (1UL << (b))

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

#if defined(_WIN32) && !defined(ORISIM_LOCALTIME_R_COMPAT)
#define ORISIM_LOCALTIME_R_COMPAT

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x800001c
#endif

inline tm* localtime_r(const time_t* timer, tm* result) {
  if (!timer || !result) {
    return nullptr;
  }

  return localtime_s(result, timer) == 0 ? result : nullptr;
}

#endif
