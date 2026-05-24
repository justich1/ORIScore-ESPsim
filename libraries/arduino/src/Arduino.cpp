#include "Arduino.h"
#include <thread>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <new>
#include <atomic>

using SimClock = std::chrono::steady_clock;

static std::recursive_mutex gTimeMutex;
static SimClock::time_point gRealBase = SimClock::now();
static unsigned long gSimBaseMs = 0;
static double gTimeScale = 1.0;
static bool gTimeScaleLoaded = false;
static std::map<uint8_t, uint8_t> gPinModes;
static std::map<uint8_t, uint8_t> gDigitalValues;
static std::map<uint8_t, int> gAnalogValues;
static unsigned long gRandomSeed = 1;
static unsigned long gManualAdvanceMillis = 0;

// ------------------------------------------------------------
// Simulovaný ESP heap
// ------------------------------------------------------------
// Cíl není měřit RAM Windows procesu, ale dát firmware realistický
// runtime pohled přes ESP.getFreeHeap() / ESP.getMaxFreeBlockSize().
// Sledujeme C++ alokace v běžícím FirmwareSim.exe přes globální new/delete.
// Kapacitu nastaví BuildService přes ESP.resetHeap(total, total) podle board JSON.

static std::atomic<unsigned long long> gSimHeapTrackedBytes{0};
static std::atomic<unsigned long long> gSimHeapAllocationCount{0};
static std::recursive_mutex gSimHeapConfigMutex;
static unsigned long gSimHeapTotalBytes = 81920;
static unsigned long long gSimHeapBaselineBytes = 0;
static unsigned long long gSimHeapBaseUsedBytes = 0;

static constexpr uint32_t SIM_ALLOC_MAGIC = 0x4F524953u; // "ORIS"
struct SimAllocHeader {
  uint32_t magic;
  size_t size;
};

static void simTrackAlloc(size_t size) noexcept {
  gSimHeapTrackedBytes.fetch_add((unsigned long long)size, std::memory_order_relaxed);
  gSimHeapAllocationCount.fetch_add(1, std::memory_order_relaxed);
}

static void simTrackFree(size_t size) noexcept {
  unsigned long long oldBytes = gSimHeapTrackedBytes.load(std::memory_order_relaxed);
  while (oldBytes > 0) {
    unsigned long long next = oldBytes > size ? oldBytes - size : 0;
    if (gSimHeapTrackedBytes.compare_exchange_weak(oldBytes, next, std::memory_order_relaxed)) break;
  }

  unsigned long long oldCount = gSimHeapAllocationCount.load(std::memory_order_relaxed);
  while (oldCount > 0) {
    unsigned long long next = oldCount - 1;
    if (gSimHeapAllocationCount.compare_exchange_weak(oldCount, next, std::memory_order_relaxed)) break;
  }
}

static unsigned long long simHeapDynamicUsedLocked() {
  unsigned long long tracked = gSimHeapTrackedBytes.load(std::memory_order_relaxed);
  return tracked > gSimHeapBaselineBytes ? tracked - gSimHeapBaselineBytes : 0;
}

static unsigned long simHeapUsedLocked() {
  unsigned long long used = gSimHeapBaseUsedBytes + simHeapDynamicUsedLocked();
  if (used > gSimHeapTotalBytes) used = gSimHeapTotalBytes;
  return (unsigned long)used;
}

static unsigned long simHeapFreeLocked() {
  unsigned long used = simHeapUsedLocked();
  return used >= gSimHeapTotalBytes ? 0 : gSimHeapTotalBytes - used;
}

static unsigned long simHeapMaxBlockLocked() {
  unsigned long freeHeap = simHeapFreeLocked();
  if (freeHeap == 0) return 0;

  unsigned long used = simHeapUsedLocked();
  unsigned long long allocs = gSimHeapAllocationCount.load(std::memory_order_relaxed);

  // Jednoduchý model fragmentace:
  // čím více aktivních alokací a čím více obsazené paměti, tím menší max blok.
  unsigned long fragPenalty = (unsigned long)std::min<unsigned long long>(
    (unsigned long long)freeHeap,
    allocs * 32ULL + (unsigned long long)(used / 16)
  );

  if (fragPenalty >= freeHeap) {
    return std::max(1UL, freeHeap / 2);
  }

  return freeHeap - fragPenalty;
}

static void* simAllocRaw(std::size_t size) {
  if (size == 0) size = 1;

  std::size_t total = sizeof(SimAllocHeader) + size;
  void* raw = std::malloc(total);
  if (!raw) throw std::bad_alloc();

  auto* header = static_cast<SimAllocHeader*>(raw);
  header->magic = SIM_ALLOC_MAGIC;
  header->size = size;

  simTrackAlloc(size);
  return header + 1;
}

static void simFreeRaw(void* ptr) noexcept {
  if (!ptr) return;

  auto* header = static_cast<SimAllocHeader*>(ptr) - 1;
  if (header->magic == SIM_ALLOC_MAGIC) {
    size_t size = header->size;
    header->magic = 0;
    header->size = 0;
    simTrackFree(size);
    std::free(header);
    return;
  }

  // Fallback pro cizí pointer, kdyby sem někdy omylem doputoval.
  std::free(ptr);
}

void* operator new(std::size_t size) {
  return simAllocRaw(size);
}

void* operator new[](std::size_t size) {
  return simAllocRaw(size);
}

void operator delete(void* ptr) noexcept {
  simFreeRaw(ptr);
}

void operator delete[](void* ptr) noexcept {
  simFreeRaw(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
  simFreeRaw(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
  simFreeRaw(ptr);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  try { return simAllocRaw(size); } catch (...) { return nullptr; }
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
  try { return simAllocRaw(size); } catch (...) { return nullptr; }
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
  simFreeRaw(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
  simFreeRaw(ptr);
}



static std::recursive_mutex gVirtualHwMutex;
static std::vector<SimVirtualDevice> gVirtualDevices;

static std::recursive_mutex& simSerialRegistryMutex() {
  static std::recursive_mutex m;
  return m;
}

static std::vector<FakeSerial*>& simSerialRegistry() {
  static std::vector<FakeSerial*> ports;
  return ports;
}

static void simRegisterSerial(FakeSerial* port) {
  if (!port) return;
  std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
  auto& ports = simSerialRegistry();
  if (std::find(ports.begin(), ports.end(), port) == ports.end()) {
    ports.push_back(port);
  }
}

static void simUnregisterSerial(FakeSerial* port) {
  std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
  auto& ports = simSerialRegistry();
  ports.erase(std::remove(ports.begin(), ports.end(), port), ports.end());
}


static String simTrimmedUpper(String s) {
  s.trim();
  s.toUpperCase();
  return s;
}

static String simNormalizePin(String pin) {
  pin.trim();
  pin.toUpperCase();
  if (pin.startsWith("GPIO")) pin = pin.substring(4);
  if (pin == "A0") return String("0");
  return pin;
}

static String simNormalizeAddress(String address) {
  String src = address;
  src.trim();
  src.toUpperCase();
  String out;
  for (size_t i = 0; i < src.length(); i++) {
    char c = src[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) out += c;
  }

  // DS18B20 ve formátu Linux/w1 bývá bez CRC bajtu: 28-000000000001 => 14 HEX.
  // Simulátor ale pracuje s Dallas DeviceAddress[8], proto doplníme poslední bajt 00.
  if (out.length() == 14 && out.startsWith("28")) out += "00";

  return out;
}

static int simParseBoolValue(const String& value) {
  String v = simTrimmedUpper(value);
  return (v == "1" || v == "TRUE" || v == "ON" || v == "HIGH") ? HIGH : LOW;
}

static int simParseAnalogRaw(const String& value) {
  String v = value;
  v.trim();
  int raw = 0;
  try {
    raw = std::stoi(v.v);
  } catch (...) {
    try {
      float f = std::stof(v.v);
      raw = (int)std::lround(std::max(0.0f, std::min(100.0f, f)) / 100.0f * 1023.0f);
    } catch (...) {
      raw = 0;
    }
  }
  if (raw < 0) raw = 0;
  if (raw > 1023) raw = 1023;
  return raw;
}


static void emitPinState(uint8_t pin) {
  int mode = gPinModes.count(pin) ? gPinModes[pin] : -1;
  int value = gDigitalValues.count(pin) ? gDigitalValues[pin] : HIGH;
  std::cout << "PINSTATE GPIO=" << (int)pin << " MODE=" << mode << " VALUE=" << value << std::endl;
}

static void setPinValue(uint8_t pin, uint8_t value) {
  uint8_t normalized = value ? HIGH : LOW;
  bool changed = !gDigitalValues.count(pin) || gDigitalValues[pin] != normalized;
  gDigitalValues[pin] = normalized;
  if (changed) emitPinState(pin);
}

FakeSerial Serial;
FakeSerial Serial1;
FakeSerial Serial2;
FakeSerial Serial3;
FakeESPClass ESP;

static std::string formatFloat(double f, int decimals) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(decimals) << f;
  return oss.str();
}

static std::string formatUnsignedBase(unsigned long long value, int base) {
  if (base < 2 || base > 36) base = 10;
  if (value == 0) return "0";

  const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::string out;

  while (value > 0) {
    out.push_back(digits[value % (unsigned long long)base]);
    value /= (unsigned long long)base;
  }

  std::reverse(out.begin(), out.end());
  return out;
}

static std::string formatSignedBase(long long value, int base) {
  if (base == 10) return std::to_string(value);
  if (value < 0) return "-" + formatUnsignedBase((unsigned long long)(-value), base);
  return formatUnsignedBase((unsigned long long)value, base);
}

String::String() : v() {}
String::String(const char* s) : v(s ? s : "") {}
String::String(const std::string& s) : v(s) {}
String::String(char c) : v(1, c) {}
String::String(int n) : v(std::to_string(n)) {}
String::String(unsigned int n) : v(std::to_string(n)) {}
String::String(long n) : v(std::to_string(n)) {}
String::String(unsigned long n) : v(std::to_string(n)) {}
String::String(long long n) : v(std::to_string(n)) {}
String::String(unsigned long long n) : v(std::to_string(n)) {}
String::String(int n, int base) : v(formatSignedBase(n, base)) {}
String::String(unsigned int n, int base) : v(formatUnsignedBase(n, base)) {}
String::String(long n, int base) : v(formatSignedBase(n, base)) {}
String::String(unsigned long n, int base) : v(formatUnsignedBase(n, base)) {}
String::String(long long n, int base) : v(formatSignedBase(n, base)) {}
String::String(unsigned long long n, int base) : v(formatUnsignedBase(n, base)) {}
String::String(float f, int decimals) : v(formatFloat(f, decimals)) {}
String::String(double f, int decimals) : v(formatFloat(f, decimals)) {}

size_t String::length() const { return v.length(); }
bool String::isEmpty() const { return v.empty(); }
bool String::reserve(size_t n) { try { v.reserve(n); return true; } catch (...) { return false; } }
size_t String::capacity() const { return v.capacity(); }
const char* String::c_str() const { return v.c_str(); }

void String::toCharArray(char* buffer, size_t size) const {
  if (!buffer || size == 0) return;
  std::strncpy(buffer, v.c_str(), size - 1);
  buffer[size - 1] = '\0';
}

bool String::concat(const String& s) { v += s.v; return true; }
bool String::concat(const char* s) { v += s ? s : ""; return true; }
bool String::concat(char c) { v.push_back(c); return true; }
bool String::concat(int n) { v += std::to_string(n); return true; }
bool String::concat(unsigned int n) { v += std::to_string(n); return true; }
bool String::concat(long n) { v += std::to_string(n); return true; }
bool String::concat(unsigned long n) { v += std::to_string(n); return true; }
int String::compareTo(const String& s) const { return v.compare(s.v); }
bool String::equals(const String& s) const { return v == s.v; }
bool String::equals(const char* s) const { return v == (s ? s : ""); }
bool String::equalsIgnoreCase(const String& s) const {
  if (v.size() != s.v.size()) return false;
  for (size_t i = 0; i < v.size(); i++) {
    if (std::tolower((unsigned char)v[i]) != std::tolower((unsigned char)s.v[i])) return false;
  }
  return true;
}

int String::indexOf(const String& needle) const { auto p = v.find(needle.v); return p == std::string::npos ? -1 : (int)p; }
int String::indexOf(const String& needle, size_t from) const { auto p = v.find(needle.v, from); return p == std::string::npos ? -1 : (int)p; }
int String::indexOf(const char* needle) const { auto p = v.find(needle ? needle : ""); return p == std::string::npos ? -1 : (int)p; }
int String::indexOf(const char* needle, size_t from) const { auto p = v.find(needle ? needle : "", from); return p == std::string::npos ? -1 : (int)p; }
int String::indexOf(char needle) const { auto p = v.find(needle); return p == std::string::npos ? -1 : (int)p; }
int String::indexOf(char needle, size_t from) const { auto p = v.find(needle, from); return p == std::string::npos ? -1 : (int)p; }
int String::lastIndexOf(const String& needle) const { auto p = v.rfind(needle.v); return p == std::string::npos ? -1 : (int)p; }
bool String::startsWith(const String& prefix) const { return v.rfind(prefix.v, 0) == 0; }
bool String::endsWith(const String& suffix) const { return v.size() >= suffix.v.size() && v.compare(v.size() - suffix.v.size(), suffix.v.size(), suffix.v) == 0; }

String String::substring(size_t from) const { if (from >= v.size()) return String(""); return String(v.substr(from)); }
String String::substring(size_t from, size_t to) const { if (from >= v.size()) return String(""); if (to < from) to = from; return String(v.substr(from, to - from)); }
void String::remove(size_t index) { if (index < v.size()) v.erase(index); }
void String::remove(size_t index, size_t count) { if (index < v.size()) v.erase(index, count); }
void String::replace(const String& from, const String& to) {
  if (from.v.empty()) return;
  size_t pos = 0;
  while ((pos = v.find(from.v, pos)) != std::string::npos) {
    v.replace(pos, from.v.length(), to.v);
    pos += to.v.length();
  }
}
void String::trim() {
  auto notSpace = [](unsigned char c){ return !std::isspace(c); };
  v.erase(v.begin(), std::find_if(v.begin(), v.end(), notSpace));
  v.erase(std::find_if(v.rbegin(), v.rend(), notSpace).base(), v.end());
}
void String::toUpperCase() { std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return (char)std::toupper(c); }); }
void String::toLowerCase() { std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return (char)std::tolower(c); }); }
int String::toInt() const { try { return std::stoi(v); } catch (...) { return 0; } }
float String::toFloat() const { try { return std::stof(v); } catch (...) { return 0.0f; } }
char String::charAt(size_t idx) const { return idx < v.size() ? v[idx] : '\0'; }
void String::setCharAt(size_t idx, char c) { if (idx < v.size()) v[idx] = c; }
char String::operator[](size_t idx) const { return idx < v.size() ? v[idx] : '\0'; }
char& String::operator[](size_t idx) { if (idx >= v.size()) v.resize(idx + 1); return v[idx]; }
String& String::operator+=(const String& other) { v += other.v; return *this; }
String& String::operator+=(const char* other) { v += other ? other : ""; return *this; }
String& String::operator+=(char c) { v += c; return *this; }
String& String::operator+=(int n) { v += std::to_string(n); return *this; }
String& String::operator+=(unsigned int n) { v += std::to_string(n); return *this; }
String& String::operator+=(long n) { v += std::to_string(n); return *this; }
String& String::operator+=(unsigned long n) { v += std::to_string(n); return *this; }
String& String::operator+=(float f) { v += formatFloat(f, 2); return *this; }
String& String::operator+=(double f) { v += formatFloat(f, 2); return *this; }
String::operator bool() const { return !v.empty(); }
String::operator std::string() const { return v; }

String operator+(const String& a, const String& b) { return String(a.v + b.v); }
String operator+(const String& a, const char* b) { return String(a.v + std::string(b ? b : "")); }
String operator+(const char* a, const String& b) { return String(std::string(a ? a : "") + b.v); }
String operator+(const String& a, char b) { return String(a.v + std::string(1, b)); }
bool operator==(const String& a, const String& b) { return a.v == b.v; }
bool operator==(const String& a, const char* b) { return a.v == (b ? b : ""); }
bool operator==(const char* a, const String& b) { return (a ? a : "") == b.v; }
bool operator!=(const String& a, const String& b) { return !(a == b); }
bool operator!=(const String& a, const char* b) { return !(a == b); }
bool operator!=(const char* a, const String& b) { return !(a == b); }
bool operator<(const String& a, const String& b) { return a.v < b.v; }
bool operator>(const String& a, const String& b) { return a.v > b.v; }
bool operator<=(const String& a, const String& b) { return a.v <= b.v; }
bool operator>=(const String& a, const String& b) { return a.v >= b.v; }
std::ostream& operator<<(std::ostream& os, const String& s) { os << s.v; return os; }

static unsigned long computeMillisLocked() {
  auto now = SimClock::now();
  auto realElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - gRealBase).count();
  if (realElapsedMs < 0) realElapsedMs = 0;
  return gSimBaseMs + (unsigned long)((double)realElapsedMs * gTimeScale);
}

static void loadTimeScaleFromEnvLocked() {
  if (gTimeScaleLoaded) return;
  gTimeScaleLoaded = true;
  const char* env = std::getenv("ORISIM_TIME_SCALE");
  if (env && *env) {
    double v = std::atof(env);
    if (v > 0.01 && v < 10000.0) gTimeScale = v;
  }
  std::cout << "SIM time scale x" << gTimeScale << std::endl;
}

double simGetTimeScale() {
  std::lock_guard<std::recursive_mutex> lock(gTimeMutex);
  loadTimeScaleFromEnvLocked();
  return gTimeScale;
}

unsigned long millis() {
  std::lock_guard<std::recursive_mutex> lock(gTimeMutex);
  loadTimeScaleFromEnvLocked();
  return computeMillisLocked();
}

void delay(unsigned long ms) {
  double scale = simGetTimeScale();
  if (scale <= 0.0) scale = 1.0;

  if (ms == 0) {
    std::this_thread::yield();
    return;
  }

  auto realSleepUs = (long long)((double)ms * 1000.0 / scale);
  if (realSleepUs < 1) realSleepUs = 1;
  std::this_thread::sleep_for(std::chrono::microseconds(realSleepUs));
}

void yield() {
  std::this_thread::yield();
}

void simSetMillis(unsigned long ms) {
  std::lock_guard<std::recursive_mutex> lock(gTimeMutex);
  loadTimeScaleFromEnvLocked();
  gSimBaseMs = ms;
  gRealBase = SimClock::now();
}

void simAdvanceMillis(unsigned long ms) {
  std::lock_guard<std::recursive_mutex> lock(gTimeMutex);
  loadTimeScaleFromEnvLocked();
  gSimBaseMs = computeMillisLocked() + ms;
  gRealBase = SimClock::now();
}

void simSetTimeScale(double scale) {
  if (scale <= 0.01 || scale >= 10000.0) scale = 1.0;
  std::lock_guard<std::recursive_mutex> lock(gTimeMutex);
  loadTimeScaleFromEnvLocked();
  unsigned long nowMs = computeMillisLocked();
  gSimBaseMs = nowMs;
  gRealBase = SimClock::now();
  gTimeScale = scale;
  std::cout << "SIM time scale x" << gTimeScale << std::endl;
}

unsigned long simConsumeManualAdvanceMillis() {
  return 0;
}


void simVirtualHwClear() {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  gVirtualDevices.clear();
  gAnalogValues.clear();
}

void simVirtualHwSet(const String& typeIn, const String& name, const String& pinIn, const String& addressIn,
                     const String& value, const String& humidity, const String& pressure, bool connected) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);

  String type = simTrimmedUpper(typeIn);
  String pin = simNormalizePin(pinIn);
  String address = type == "DS18B20" ? simNormalizeAddress(addressIn) : addressIn;

  SimVirtualDevice d{ type, name, pin, address, value, humidity, pressure, connected };

  auto sameDevice = [&](const SimVirtualDevice& x) {
    if (type == "DS18B20") return x.type == type && x.address == address;
    if (type == "DHT11" || type == "DHT22" || type == "AM2302" || type == "BME280" || type == "BMP280" || type == "BMP180" || type == "SHT31" || type == "AHT20") return x.type == type && x.pin == pin && x.address == address;
    return x.type == type && x.name == name && x.pin == pin;
  };

  bool updated = false;
  for (auto& x : gVirtualDevices) {
    if (sameDevice(x)) {
      x = d;
      updated = true;
      break;
    }
  }
  if (!updated) gVirtualDevices.push_back(d);

  if (type == "DIGITAL_INPUT") {
    int gpio = pin.toInt();
    if (gpio >= 0 && gpio <= 255) setPinValue((uint8_t)gpio, (uint8_t)simParseBoolValue(value));
  }

  if (type.startsWith("ANALOG") || type == "MQ135") {
    int gpio = pin.toInt();
    if (gpio >= 0 && gpio <= 255) gAnalogValues[(uint8_t)gpio] = simParseAnalogRaw(value);
  }
}

int simVirtualHwCount(const String& typeIn) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  String type = simTrimmedUpper(typeIn);
  int count = 0;
  for (const auto& d : gVirtualDevices) {
    if (d.type == type && d.connected) count++;
  }
  return count;
}

bool simVirtualHwGetByTypeIndex(const String& typeIn, int index, SimVirtualDevice* out) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  String type = simTrimmedUpper(typeIn);
  int cur = 0;
  for (const auto& d : gVirtualDevices) {
    if (d.type == type && d.connected) {
      if (cur == index) {
        if (out) *out = d;
        return true;
      }
      cur++;
    }
  }
  return false;
}

bool simVirtualHwFindDs18b20ByAddress(const String& addressIn, SimVirtualDevice* out) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  String address = simNormalizeAddress(addressIn);
  for (const auto& d : gVirtualDevices) {
    if (d.type == "DS18B20" && d.address == address) {
      if (out) *out = d;
      return true;
    }
  }
  return false;
}

bool simVirtualHwFindDhtByPin(uint8_t pin, SimVirtualDevice* out) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  String pinText((int)pin);
  for (const auto& d : gVirtualDevices) {
    if ((d.type == "DHT11" || d.type == "DHT22" || d.type == "AM2302") && d.pin == pinText) {
      if (out) *out = d;
      return true;
    }
  }
  return false;
}

int simVirtualHwAnalogRaw(uint8_t pin) {
  std::lock_guard<std::recursive_mutex> lock(gVirtualHwMutex);
  auto it = gAnalogValues.find(pin);
  return it == gAnalogValues.end() ? 0 : it->second;
}

void pinMode(uint8_t pin, uint8_t mode) {
  bool changed = !gPinModes.count(pin) || gPinModes[pin] != mode;
  gPinModes[pin] = mode;
  if (mode == INPUT_PULLUP && !gDigitalValues.count(pin)) {
    gDigitalValues[pin] = HIGH;
    changed = true;
  }
  else if (!gDigitalValues.count(pin)) {
    gDigitalValues[pin] = HIGH;
    changed = true;
  }
  if (changed) emitPinState(pin);
}

void digitalWrite(uint8_t pin, uint8_t value) {
  setPinValue(pin, value);
}

int digitalRead(uint8_t pin) {
  auto it = gDigitalValues.find(pin);
  return it == gDigitalValues.end() ? HIGH : it->second;
}

void simSetDigitalInput(uint8_t pin, uint8_t value) {
  setPinValue(pin, value);
}

int simGetDigitalOutput(uint8_t pin) { return digitalRead(pin); }
int simGetPinMode(uint8_t pin) { auto it = gPinModes.find(pin); return it == gPinModes.end() ? -1 : it->second; }
int simGetDigitalValue(uint8_t pin) { return digitalRead(pin); }
int analogRead(uint8_t pin) {
  auto it = gAnalogValues.find(pin);
  return it == gAnalogValues.end() ? 0 : it->second;
}

void simSetAnalogInput(uint8_t pin, int value) {
  if (value < 0) value = 0;
  if (value > 1023) value = 1023;
  gAnalogValues[pin] = value;
  std::cout << "ANALOGSTATE GPIO=" << (int)pin << " RAW=" << value << std::endl;
}

void simClearAnalogInputs() {
  gAnalogValues.clear();
  std::cout << "ANALOGSTATE CLEAR" << std::endl;
}
void analogWrite(uint8_t pin, int value) { setPinValue(pin, (uint8_t)(value ? HIGH : LOW)); }

void attachInterrupt(uint8_t pin, void (*callback)(), int mode) {
  (void)pin; (void)callback; (void)mode;
}

void detachInterrupt(uint8_t pin) {
  (void)pin;
}

void noInterrupts() {}
void interrupts() {}

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) {
  (void)pin; (void)state; (void)timeout;
  return 0;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
  (void)dataPin; (void)clockPin; (void)bitOrder; (void)val;
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
  (void)dataPin; (void)clockPin; (void)bitOrder;
  return 0;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long random(long max) { return max <= 0 ? 0 : rand() % max; }
long random(long min, long max) { return min + random(max - min); }
void randomSeed(unsigned long seed) { gRandomSeed = seed; srand((unsigned int)seed); }
uint8_t highByte(uint16_t w) { return (uint8_t)(w >> 8); }
uint8_t lowByte(uint16_t w) { return (uint8_t)(w & 0xff); }


size_t Print::write(const uint8_t* buffer, size_t size) {
  if (!buffer) return 0;
  size_t n = 0;
  while (size--) {
    n += write(*buffer++);
  }
  return n;
}

size_t Print::write(const char* str) {
  if (!str) return 0;
  return write((const uint8_t*)str, std::strlen(str));
}

size_t Print::write(const char* buffer, size_t size) {
  return write((const uint8_t*)buffer, size);
}

size_t Print::write(const String& s) {
  return write(s.c_str());
}

size_t Print::printNumber(unsigned long long n, uint8_t base) {
  if (base < 2) base = 10;
  char buf[8 * sizeof(unsigned long long) + 1];
  char* str = &buf[sizeof(buf) - 1];
  *str = '\0';

  do {
    unsigned long long m = n;
    n /= base;
    char c = (char)(m - base * n);
    *--str = (char)(c < 10 ? c + '0' : c + 'A' - 10);
  } while (n);

  return write(str);
}

size_t Print::printFloat(double number, uint8_t digits) {
  if (std::isnan(number)) return print("nan");
  if (std::isinf(number)) return print("inf");
  if (number > 4294967040.0) return print("ovf");
  if (number < -4294967040.0) return print("ovf");

  size_t n = 0;
  if (number < 0.0) {
    n += write((uint8_t)'-');
    number = -number;
  }

  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i) rounding /= 10.0;
  number += rounding;

  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  n += print(int_part);

  if (digits > 0) n += write((uint8_t)'.');

  while (digits-- > 0) {
    remainder *= 10.0;
    unsigned int toPrint = (unsigned int)remainder;
    n += print(toPrint);
    remainder -= toPrint;
  }

  return n;
}

size_t Print::print(const __FlashStringHelper* s) { return print((const char*)s); }
size_t Print::print(const String& s) { return write(s.c_str()); }
size_t Print::print(const char s[]) { return write(s); }
size_t Print::print(char c) { return write((uint8_t)c); }
size_t Print::print(unsigned char b, int base) { return print((unsigned long)b, base); }

size_t Print::print(int n, int base) {
  return print((long)n, base);
}

size_t Print::print(unsigned int n, int base) {
  return print((unsigned long)n, base);
}

size_t Print::print(long n, int base) {
  if (base == 0) return write((uint8_t)n);
  if (base == 10 && n < 0) {
    size_t t = write((uint8_t)'-');
    return t + printNumber((unsigned long long)(-n), 10);
  }
  return printNumber((unsigned long long)n, (uint8_t)base);
}

size_t Print::print(unsigned long n, int base) {
  if (base == 0) return write((uint8_t)n);
  return printNumber((unsigned long long)n, (uint8_t)base);
}

size_t Print::print(long long n, int base) {
  if (base == 10 && n < 0) {
    size_t t = write((uint8_t)'-');
    return t + printNumber((unsigned long long)(-n), 10);
  }
  return printNumber((unsigned long long)n, (uint8_t)base);
}

size_t Print::print(unsigned long long n, int base) {
  return printNumber(n, (uint8_t)base);
}

size_t Print::print(float f, int digits) { return printFloat((double)f, (uint8_t)digits); }
size_t Print::print(double f, int digits) { return printFloat(f, (uint8_t)digits); }
size_t Print::print(const Printable& x) { return x.printTo(*this); }

size_t Print::println(const __FlashStringHelper* s) { size_t n = print(s); n += println(); return n; }
size_t Print::println(const String& s) { size_t n = print(s); n += println(); return n; }
size_t Print::println(const char s[]) { size_t n = print(s); n += println(); return n; }
size_t Print::println(char c) { size_t n = print(c); n += println(); return n; }
size_t Print::println(unsigned char b, int base) { size_t n = print(b, base); n += println(); return n; }
size_t Print::println(int n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(unsigned int n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(long n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(unsigned long n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(long long n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(unsigned long long n, int base) { size_t r = print(n, base); r += println(); return r; }
size_t Print::println(float f, int digits) { size_t r = print(f, digits); r += println(); return r; }
size_t Print::println(double f, int digits) { size_t r = print(f, digits); r += println(); return r; }
size_t Print::println(const Printable& x) { size_t n = print(x); n += println(); return n; }
size_t Print::println() { return write("\r\n"); }

void Stream::setTimeout(unsigned long timeout) { _timeout = timeout; }

int Stream::timedRead() {
  _startMillis = millis();
  do {
    int c = read();
    if (c >= 0) return c;
    yield();
  } while (millis() - _startMillis < _timeout);
  return -1;
}

int Stream::timedPeek() {
  _startMillis = millis();
  do {
    int c = peek();
    if (c >= 0) return c;
    yield();
  } while (millis() - _startMillis < _timeout);
  return -1;
}

int Stream::peekNextDigit(LookaheadMode lookahead, bool detectDecimal) {
  int c;
  while (true) {
    c = timedPeek();
    if (c < 0) return c;
    if (c == '-' || (c >= '0' && c <= '9') || (detectDecimal && c == '.')) return c;

    switch (lookahead) {
      case SKIP_NONE: return -1;
      case SKIP_WHITESPACE:
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return -1;
        break;
      case SKIP_ALL:
        break;
    }
    read();
  }
}

long Stream::parseInt(LookaheadMode lookahead, char ignore) {
  bool isNegative = false;
  long value = 0;
  int c = peekNextDigit(lookahead, false);
  if (c < 0) return 0;

  do {
    if (c == ignore) {
    } else if (c == '-') {
      isNegative = true;
    } else if (c >= '0' && c <= '9') {
      value = value * 10 + c - '0';
    }
    read();
    c = timedPeek();
  } while ((c >= '0' && c <= '9') || c == ignore);

  return isNegative ? -value : value;
}

float Stream::parseFloat(LookaheadMode lookahead, char ignore) {
  bool isNegative = false;
  bool isFraction = false;
  double value = 0.0;
  double fraction = 1.0;

  int c = peekNextDigit(lookahead, true);
  if (c < 0) return 0.0f;

  do {
    if (c == ignore) {
    } else if (c == '-') {
      isNegative = true;
    } else if (c == '.') {
      isFraction = true;
    } else if (c >= '0' && c <= '9') {
      if (isFraction) {
        fraction *= 0.1;
        value += (c - '0') * fraction;
      } else {
        value = value * 10.0 + c - '0';
      }
    }
    read();
    c = timedPeek();
  } while ((c >= '0' && c <= '9') || c == '.' || c == ignore);

  if (isNegative) value = -value;
  return (float)value;
}

size_t Stream::readBytes(char* buffer, size_t length) {
  size_t count = 0;
  while (count < length) {
    int c = timedRead();
    if (c < 0) break;
    *buffer++ = (char)c;
    count++;
  }
  return count;
}

size_t Stream::readBytesUntil(char terminator, char* buffer, size_t length) {
  if (length < 1) return 0;
  size_t index = 0;
  while (index < length) {
    int c = timedRead();
    if (c < 0 || c == terminator) break;
    *buffer++ = (char)c;
    index++;
  }
  return index;
}

String Stream::readString() {
  String ret;
  int c = timedRead();
  while (c >= 0) {
    ret += (char)c;
    c = timedRead();
  }
  return ret;
}

String Stream::readStringUntil(char terminator) {
  String ret;
  int c = timedRead();
  while (c >= 0 && c != terminator) {
    ret += (char)c;
    c = timedRead();
  }
  return ret;
}

bool Stream::find(const char* target) {
  return target ? find(target, std::strlen(target)) : false;
}

bool Stream::find(const char* target, size_t length) {
  if (!target || length == 0) return false;
  size_t index = 0;
  _startMillis = millis();
  while (millis() - _startMillis < _timeout) {
    int c = read();
    if (c < 0) {
      yield();
      continue;
    }
    if ((char)c == target[index]) {
      if (++index >= length) return true;
    } else {
      index = ((char)c == target[0]) ? 1 : 0;
    }
  }
  return false;
}

bool Stream::findUntil(const char* target, const char* terminator) {
  return findUntil(target, target ? std::strlen(target) : 0, terminator, terminator ? std::strlen(terminator) : 0);
}

bool Stream::findUntil(const char* target, size_t targetLen, const char* terminate, size_t termLen) {
  if (!target || targetLen == 0) return false;
  size_t targetIndex = 0;
  size_t termIndex = 0;
  _startMillis = millis();
  while (millis() - _startMillis < _timeout) {
    int c = read();
    if (c < 0) {
      yield();
      continue;
    }

    if ((char)c == target[targetIndex]) {
      if (++targetIndex >= targetLen) return true;
    } else {
      targetIndex = ((char)c == target[0]) ? 1 : 0;
    }

    if (terminate && termLen > 0) {
      if ((char)c == terminate[termIndex]) {
        if (++termIndex >= termLen) return false;
      } else {
        termIndex = ((char)c == terminate[0]) ? 1 : 0;
      }
    }
  }
  return false;
}

IPAddress::IPAddress() { _address.dword = 0; }
IPAddress::IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) {
  _address.bytes[0] = first;
  _address.bytes[1] = second;
  _address.bytes[2] = third;
  _address.bytes[3] = fourth;
}
IPAddress::IPAddress(uint32_t address) { _address.dword = address; }
IPAddress::IPAddress(const uint8_t* address) { *this = address; }

bool IPAddress::fromString(const char* address) {
  if (!address) return false;
  int parts[4] = {0, 0, 0, 0};
  char extra = 0;
  if (std::sscanf(address, "%d.%d.%d.%d%c", &parts[0], &parts[1], &parts[2], &parts[3], &extra) != 4) {
    return false;
  }
  for (int i = 0; i < 4; i++) {
    if (parts[i] < 0 || parts[i] > 255) return false;
    _address.bytes[i] = (uint8_t)parts[i];
  }
  return true;
}

bool IPAddress::operator==(const uint8_t* addr) const {
  if (!addr) return false;
  for (int i = 0; i < 4; i++) {
    if (_address.bytes[i] != addr[i]) return false;
  }
  return true;
}

IPAddress& IPAddress::operator=(const uint8_t* address) {
  if (address) {
    for (int i = 0; i < 4; i++) _address.bytes[i] = address[i];
  } else {
    _address.dword = 0;
  }
  return *this;
}

IPAddress& IPAddress::operator=(uint32_t address) {
  _address.dword = address;
  return *this;
}

String IPAddress::toString() const {
  return String((int)_address.bytes[0]) + "." +
         String((int)_address.bytes[1]) + "." +
         String((int)_address.bytes[2]) + "." +
         String((int)_address.bytes[3]);
}

size_t IPAddress::printTo(Print& p) const {
  return p.print(toString());
}

#ifdef INADDR_NONE
#undef INADDR_NONE
#endif
const IPAddress INADDR_NONE(0, 0, 0, 0);


FakeSerial::FakeSerial() {
  simRegisterSerial(this);
}

FakeSerial::FakeSerial(int uartNo) {
  (void)uartNo;
  simRegisterSerial(this);
}

FakeSerial::~FakeSerial() {
  simUnregisterSerial(this);
}

void FakeSerial::begin(uint32_t baud) {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  baudRate = baud;
}

void FakeSerial::begin(uint32_t baud, uint32_t config, int8_t rxPin, int8_t txPin) {
  (void)config;
  (void)rxPin;
  (void)txPin;
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  baudRate = baud;
}

void FakeSerial::end() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  baudRate = 0;
}

int FakeSerial::available() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  return (int)rx.size();
}

int FakeSerial::read() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  if (rx.empty()) return -1;
  char c = rx.front();
  rx.pop_front();
  return (unsigned char)c;
}

int FakeSerial::peek() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  if (rx.empty()) return -1;
  return (unsigned char)rx.front();
}

void FakeSerial::flush() {}

size_t FakeSerial::write(uint8_t b) {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx.push_back((char)b);
  return 1;
}

size_t FakeSerial::write(const uint8_t* buffer, size_t size) {
  if (!buffer) return 0;
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx.append((const char*)buffer, size);
  return size;
}

size_t FakeSerial::print(const __FlashStringHelper* s) { return Print::print(s); }

size_t FakeSerial::print(const String& s) {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx += s.v;
  return s.length();
}

size_t FakeSerial::print(const char* s) {
  std::string x = s ? s : "";
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx += x;
  return x.size();
}

size_t FakeSerial::print(char c) {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx += c;
  return 1;
}

size_t FakeSerial::print(unsigned char n) { return Print::print(n, DEC); }
size_t FakeSerial::print(int n) { return Print::print(n, DEC); }
size_t FakeSerial::print(unsigned int n) { return Print::print(n, DEC); }
size_t FakeSerial::print(long n) { return Print::print(n, DEC); }
size_t FakeSerial::print(unsigned long n) { return Print::print(n, DEC); }
size_t FakeSerial::print(long long n) { return Print::print(n, DEC); }
size_t FakeSerial::print(unsigned long long n) { return Print::print(n, DEC); }
size_t FakeSerial::print(float f) { return Print::print(f, 2); }
size_t FakeSerial::print(double f) { return Print::print(f, 2); }
size_t FakeSerial::print(const Printable& x) { return Print::print(x); }

size_t FakeSerial::print(unsigned char n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(int n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(unsigned int n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(long n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(unsigned long n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(long long n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(unsigned long long n, int base) { return Print::print(n, base); }
size_t FakeSerial::print(float f, int digits) { return Print::print(f, digits); }
size_t FakeSerial::print(double f, int digits) { return Print::print(f, digits); }

size_t FakeSerial::println() { return print("\r\n"); }
size_t FakeSerial::println(const __FlashStringHelper* s) { return Print::println(s); }
size_t FakeSerial::println(const String& s) { size_t n = print(s); n += println(); return n; }
size_t FakeSerial::println(const char* s) { size_t n = print(s); n += println(); return n; }
size_t FakeSerial::println(char c) { size_t n = print(c); n += println(); return n; }
size_t FakeSerial::println(unsigned char n) { return Print::println(n, DEC); }
size_t FakeSerial::println(int n) { return Print::println(n, DEC); }
size_t FakeSerial::println(unsigned int n) { return Print::println(n, DEC); }
size_t FakeSerial::println(long n) { return Print::println(n, DEC); }
size_t FakeSerial::println(unsigned long n) { return Print::println(n, DEC); }
size_t FakeSerial::println(long long n) { return Print::println(n, DEC); }
size_t FakeSerial::println(unsigned long long n) { return Print::println(n, DEC); }
size_t FakeSerial::println(float f) { return Print::println(f, 2); }
size_t FakeSerial::println(double f) { return Print::println(f, 2); }
size_t FakeSerial::println(const Printable& x) { return Print::println(x); }

size_t FakeSerial::println(unsigned char n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(int n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(unsigned int n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(long n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(unsigned long n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(long long n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(unsigned long long n, int base) { return Print::println(n, base); }
size_t FakeSerial::println(float f, int digits) { return Print::println(f, digits); }
size_t FakeSerial::println(double f, int digits) { return Print::println(f, digits); }

size_t FakeSerial::printf(const char* fmt, ...) {
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt ? fmt : "", ap);
  va_end(ap);
  if (n < 0) return 0;
  return print(buf);
}

void FakeSerial::injectRx(const String& s) {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  for (char c : s.v) rx.push_back(c);
}

String FakeSerial::takeTxLog() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  String out(tx);
  tx.clear();
  return out;
}

String FakeSerial::getTxLog() const {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  return String(tx);
}

void FakeSerial::clearTxLog() {
  std::lock_guard<std::recursive_mutex> lock(serialMutex);
  tx.clear();
}

void simInjectSerialRxAll(const String& s) {
  std::vector<FakeSerial*> ports;
  {
    std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
    ports = simSerialRegistry();
  }

  for (FakeSerial* port : ports) {
    if (port) port->injectRx(s);
  }
}

int simGetSerialPortCount() {
  std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
  return (int)simSerialRegistry().size();
}

String simTakeSerialTxByIndex(int index) {
  FakeSerial* port = nullptr;
  {
    std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
    auto& ports = simSerialRegistry();
    if (index < 0 || index >= (int)ports.size()) return String("");
    port = ports[(size_t)index];
  }

  return port ? port->takeTxLog() : String("");
}

void simClearSerialTxAll() {
  std::vector<FakeSerial*> ports;
  {
    std::lock_guard<std::recursive_mutex> lock(simSerialRegistryMutex());
    ports = simSerialRegistry();
  }

  for (FakeSerial* port : ports) {
    if (port) port->clearTxLog();
  }
}

void FakeESPClass::restart() { std::cout << "[ESP.restart()]\n"; }

uint32_t FakeESPClass::getFreeHeap() {
  std::lock_guard<std::recursive_mutex> lock(gSimHeapConfigMutex);
  fakeFreeHeap = (uint32_t)simHeapFreeLocked();
  return fakeFreeHeap;
}

uint32_t FakeESPClass::getMaxFreeBlockSize() {
  std::lock_guard<std::recursive_mutex> lock(gSimHeapConfigMutex);
  fakeMaxBlock = (uint32_t)simHeapMaxBlockLocked();
  return fakeMaxBlock;
}

uint32_t FakeESPClass::getFreeSketchSpace() { return 1024 * 1024; }
uint32_t FakeESPClass::getSketchSize() { return 256 * 1024; }
uint32_t FakeESPClass::getFlashChipSize() { return 4 * 1024 * 1024; }
uint64_t FakeESPClass::getEfuseMac() { return 0x112233445566ULL; }
String FakeESPClass::getResetReason() { return "Fake reset"; }
void FakeESPClass::wdtFeed() {}

void FakeESPClass::resetHeap(uint32_t freeHeap, uint32_t maxBlock) {
  std::lock_guard<std::recursive_mutex> lock(gSimHeapConfigMutex);

  if (freeHeap > 0) {
    gSimHeapTotalBytes = std::max<unsigned long>(1024UL, (unsigned long)freeHeap);
  }

  unsigned long requestedFree = freeHeap > 0 ? std::min<unsigned long>((unsigned long)freeHeap, gSimHeapTotalBytes) : gSimHeapTotalBytes;

  gSimHeapBaselineBytes = gSimHeapTrackedBytes.load(std::memory_order_relaxed);
  gSimHeapBaseUsedBytes = gSimHeapTotalBytes > requestedFree ? gSimHeapTotalBytes - requestedFree : 0;

  fakeFreeHeap = (uint32_t)simHeapFreeLocked();
  fakeMaxBlock = maxBlock > 0
    ? std::min<uint32_t>(maxBlock, (uint32_t)fakeFreeHeap)
    : (uint32_t)simHeapMaxBlockLocked();
}


static long gConfigTimeGmtOffset = 0;
static int gConfigTimeDstOffset = 0;
static String gConfigTimeServer = "";

void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1) {
  gConfigTimeGmtOffset = gmtOffset_sec;
  gConfigTimeDstOffset = daylightOffset_sec;
  gConfigTimeServer = server1 ? server1 : "";
  Serial.println(String("[SIM] configTime server=") + gConfigTimeServer +
                 " offset=" + String(gmtOffset_sec + daylightOffset_sec));
}

void configTime(long gmtOffset_sec, int daylightOffset_sec, const char* server1, const char* server2) {
  (void)server2;
  configTime(gmtOffset_sec, daylightOffset_sec, server1);
}

bool getLocalTime(struct tm* info, uint32_t ms) {
  (void)ms;
  if (!info) return false;
  time_t now = time(nullptr) + gConfigTimeGmtOffset + gConfigTimeDstOffset;
#if defined(_WIN32)
  localtime_s(info, &now);
#else
  localtime_r(&now, info);
#endif
  return true;
}


size_t FakeSerial::println(const struct tm* timeinfo, const char* format) {
  if (!timeinfo || !format) {
    return println("");
  }

  char buf[128];
  size_t n = std::strftime(buf, sizeof(buf), format, timeinfo);
  if (n == 0) {
    return println("");
  }

  return println(String(buf));
}
