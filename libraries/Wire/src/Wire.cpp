#include "Wire.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
constexpr uint8_t kPcf8563Address = 0x51;

uint8_t bcdToDec(uint8_t value) {
  return static_cast<uint8_t>(((value >> 4) * 10) + (value & 0x0F));
}

uint8_t decToBcd(unsigned value) {
  return static_cast<uint8_t>(((value / 10) << 4) | (value % 10));
}

// Days since 1970-01-01. This is calendar arithmetic without timezone/DST.
int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}

void civilFromDays(int64_t days, int& year, unsigned& month, unsigned& day) {
  days += 719468;
  const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(days - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  year = static_cast<int>(yoe) + static_cast<int>(era) * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  day = doy - (153 * mp + 2) / 5 + 1;
  month = mp + (mp < 10 ? 3 : -9);
  year += month <= 2;
}

uint64_t calendarToSeconds(int year, unsigned month, unsigned day,
                           unsigned hour, unsigned minute, unsigned second) {
  const int64_t epoch2000 = daysFromCivil(2000, 1, 1);
  int64_t days = daysFromCivil(year, month, day) - epoch2000;
  if (days < 0) days = 0;

  return static_cast<uint64_t>(days) * 86400ULL +
         static_cast<uint64_t>(hour) * 3600ULL +
         static_cast<uint64_t>(minute) * 60ULL +
         static_cast<uint64_t>(second);
}

void secondsToCalendar(uint64_t seconds, int& year, unsigned& month, unsigned& day,
                       unsigned& hour, unsigned& minute, unsigned& second,
                       unsigned& weekday) {
  const uint64_t wholeDays = seconds / 86400ULL;
  uint64_t remainder = seconds % 86400ULL;

  hour = static_cast<unsigned>(remainder / 3600ULL);
  remainder %= 3600ULL;
  minute = static_cast<unsigned>(remainder / 60ULL);
  second = static_cast<unsigned>(remainder % 60ULL);

  const int64_t epoch2000 = daysFromCivil(2000, 1, 1);
  const int64_t absoluteDays = epoch2000 + static_cast<int64_t>(wholeDays);
  civilFromDays(absoluteDays, year, month, day);

  // 1970-01-01 was Thursday. PCF8563 uses 0=Sunday ... 6=Saturday.
  int64_t wd = (absoluteDays + 4) % 7;
  if (wd < 0) wd += 7;
  weekday = static_cast<unsigned>(wd);
}

bool validDateTime(int year, unsigned month, unsigned day,
                   unsigned hour, unsigned minute, unsigned second) {
  return year >= 2000 && year <= 2099 &&
         month >= 1 && month <= 12 &&
         day >= 1 && day <= 31 &&
         hour <= 23 && minute <= 59 && second <= 59;
}
}  // namespace

TwoWire Wire;

TwoWire::TwoWire() = default;
TwoWire::TwoWire(int bus) : _bus(bus) {}

void TwoWire::begin() {
  ensureRtcInitialized();
  Serial.println("[SIM] Wire.begin");
}

void TwoWire::begin(int sda, int scl) {
  _sda = sda;
  _scl = scl;
  ensureRtcInitialized();
  Serial.println(String("[SIM] Wire.begin SDA=") + String(sda) + " SCL=" + String(scl));
}

void TwoWire::setClock(uint32_t clock) {
  _clock = clock;
}

void TwoWire::beginTransmission(uint8_t address) {
  _address = address;
  _tx.clear();
  if (_address == kPcf8563Address) ensureRtcInitialized();
}

size_t TwoWire::write(uint8_t b) {
  _tx.push_back(b);
  return 1;
}

size_t TwoWire::write(const uint8_t* data, size_t len) {
  if (!data) return 0;
  _tx.insert(_tx.end(), data, data + len);
  return len;
}

uint8_t TwoWire::endTransmission(bool stopBit) {
  (void)stopBit;

  Serial.println(String("[SIM] Wire.endTransmission addr=0x") +
                 String(static_cast<int>(_address), HEX) +
                 " bytes=" + String(static_cast<int>(_tx.size())));

  if (_address == kPcf8563Address && !_tx.empty()) {
    _registerPointer = _tx.front();
    if (_tx.size() > 1) {
      applyRtcWrite(_registerPointer, _tx.data() + 1, _tx.size() - 1);
      _registerPointer = static_cast<uint8_t>(_registerPointer + _tx.size() - 1);
    }
  }

  // The simulator currently acknowledges all I2C addresses so existing
  // sensor stubs keep behaving as before.
  return 0;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
  _address = address;
  _rx.clear();

  if (_address == kPcf8563Address) {
    ensureRtcInitialized();
    updateRtcRegisters();

    for (uint8_t i = 0; i < quantity; ++i) {
      _rx.push_back(_rtcRegisters[_registerPointer]);
      ++_registerPointer;
    }
  } else {
    _rx.assign(quantity, 0);
  }

  return quantity;
}

size_t TwoWire::requestFrom(int address, int quantity) {
  if (quantity < 0) quantity = 0;
  if (quantity > 255) quantity = 255;
  return requestFrom(static_cast<uint8_t>(address), static_cast<uint8_t>(quantity));
}

int TwoWire::available() {
  return static_cast<int>(_rx.size());
}

int TwoWire::read() {
  if (_rx.empty()) return -1;
  const uint8_t b = _rx.front();
  _rx.erase(_rx.begin());
  return b;
}

void TwoWire::ensureRtcInitialized() {
  if (_rtcInitialized) return;
  _rtcInitialized = true;

  std::fill(std::begin(_rtcRegisters), std::end(_rtcRegisters), 0);

  // Start from the host's local date/time. VL remains set so firmware which
  // checks lostPower() can initialize the RTC exactly as on real hardware.
  std::time_t hostNow = std::time(nullptr);
  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &hostNow);
#else
  localtime_r(&hostNow, &local);
#endif

  int year = local.tm_year + 1900;
  unsigned month = static_cast<unsigned>(local.tm_mon + 1);
  unsigned day = static_cast<unsigned>(local.tm_mday);
  unsigned hour = static_cast<unsigned>(local.tm_hour);
  unsigned minute = static_cast<unsigned>(local.tm_min);
  unsigned second = static_cast<unsigned>(local.tm_sec);

  if (!validDateTime(year, month, day, hour, minute, second)) {
    year = 2026;
    month = 1;
    day = 1;
    hour = minute = second = 0;
  }

  _rtcBaseSeconds = calendarToSeconds(year, month, day, hour, minute, second);
  _rtcBaseMillis = millis();
  _rtcRunning = true;
  _rtcVoltageLow = true;
  _rtcRegisters[0x00] = 0x00;
  _rtcRegisters[0x01] = 0x00;

  loadRtcState();
  updateRtcRegisters();
}

void TwoWire::updateRtcRegisters() {
  if (!_rtcInitialized) return;

  uint64_t currentSeconds = _rtcBaseSeconds;
  if (_rtcRunning) {
    const unsigned long elapsedMillis = millis() - _rtcBaseMillis;
    currentSeconds += static_cast<uint64_t>(elapsedMillis / 1000UL);
  }

  int year = 2000;
  unsigned month = 1, day = 1, hour = 0, minute = 0, second = 0, weekday = 6;
  secondsToCalendar(currentSeconds, year, month, day, hour, minute, second, weekday);

  _rtcRegisters[0x02] = static_cast<uint8_t>(decToBcd(second) | (_rtcVoltageLow ? 0x80 : 0x00));
  _rtcRegisters[0x03] = static_cast<uint8_t>(decToBcd(minute) & 0x7F);
  _rtcRegisters[0x04] = static_cast<uint8_t>(decToBcd(hour) & 0x3F);
  _rtcRegisters[0x05] = static_cast<uint8_t>(decToBcd(day) & 0x3F);
  _rtcRegisters[0x06] = static_cast<uint8_t>(weekday & 0x07);
  _rtcRegisters[0x07] = static_cast<uint8_t>(decToBcd(month) & 0x1F);
  _rtcRegisters[0x08] = decToBcd(static_cast<unsigned>(year - 2000));
  saveRtcState(false);
}

void TwoWire::applyRtcWrite(uint8_t firstRegister, const uint8_t* data, size_t length) {
  if (!data || length == 0) return;

  updateRtcRegisters();

  const bool wasRunning = _rtcRunning;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t reg = static_cast<uint8_t>(firstRegister + i);
    _rtcRegisters[reg] = data[i];
  }

  _rtcRunning = (_rtcRegisters[0x00] & 0x20) == 0;
  _rtcVoltageLow = (_rtcRegisters[0x02] & 0x80) != 0;

  // Any write touching clock registers commits the complete visible calendar.
  const uint16_t lastRegister = static_cast<uint16_t>(firstRegister) +
                                static_cast<uint16_t>(length) - 1;
  if (firstRegister <= 0x08 && lastRegister >= 0x02) {
    setRtcFromRegisters();
  } else if (wasRunning != _rtcRunning) {
    // STOP/resume was changed without rewriting calendar registers.
    if (!_rtcRunning) {
      updateRtcRegisters();
      setRtcFromRegisters();
    } else {
      _rtcBaseMillis = millis();
    }
  }
}

void TwoWire::setRtcFromRegisters() {
  const unsigned second = bcdToDec(static_cast<uint8_t>(_rtcRegisters[0x02] & 0x7F));
  const unsigned minute = bcdToDec(static_cast<uint8_t>(_rtcRegisters[0x03] & 0x7F));
  const unsigned hour = bcdToDec(static_cast<uint8_t>(_rtcRegisters[0x04] & 0x3F));
  const unsigned day = bcdToDec(static_cast<uint8_t>(_rtcRegisters[0x05] & 0x3F));
  const unsigned month = bcdToDec(static_cast<uint8_t>(_rtcRegisters[0x07] & 0x1F));
  const int year = 2000 + bcdToDec(_rtcRegisters[0x08]);

  if (validDateTime(year, month, day, hour, minute, second)) {
    _rtcBaseSeconds = calendarToSeconds(year, month, day, hour, minute, second);
    _rtcBaseMillis = millis();
    saveRtcState(true);
  }
}

void TwoWire::loadRtcState() {
  const char* root = std::getenv("ORISIM_FS_ROOT");
  if (!root || !*root) return;

  try {
    const std::filesystem::path path =
        std::filesystem::path(root) / "orisim_pcf8563_rtc.txt";
    std::ifstream input(path);
    if (!input) return;

    uint64_t savedSeconds = 0;
    long long savedHost = 0;
    int running = 1;
    int voltageLow = 0;
    input >> savedSeconds >> savedHost >> running >> voltageLow;
    if (!input) return;

    const long long hostNow = static_cast<long long>(std::time(nullptr));
    if (running != 0 && hostNow > savedHost) {
      savedSeconds += static_cast<uint64_t>(hostNow - savedHost);
    }

    _rtcBaseSeconds = savedSeconds;
    _rtcBaseMillis = millis();
    _rtcRunning = running != 0;
    _rtcVoltageLow = voltageLow != 0;
    _rtcLastPersistHost = hostNow;
    if (_rtcRunning) _rtcRegisters[0x00] &= static_cast<uint8_t>(~0x20);
    else _rtcRegisters[0x00] |= 0x20;
  } catch (...) {
    // Persistence is optional; RTC emulation still works in memory.
  }
}

void TwoWire::saveRtcState(bool force) {
  const char* root = std::getenv("ORISIM_FS_ROOT");
  if (!root || !*root || !_rtcInitialized) return;

  const long long hostNow = static_cast<long long>(std::time(nullptr));
  if (!force && hostNow == _rtcLastPersistHost) return;

  uint64_t currentSeconds = _rtcBaseSeconds;
  if (_rtcRunning) {
    const unsigned long elapsedMillis = millis() - _rtcBaseMillis;
    currentSeconds += static_cast<uint64_t>(elapsedMillis / 1000UL);
  }

  try {
    const std::filesystem::path path =
        std::filesystem::path(root) / "orisim_pcf8563_rtc.txt";
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output) return;
    output << currentSeconds << ' ' << hostNow << ' '
           << (_rtcRunning ? 1 : 0) << ' '
           << (_rtcVoltageLow ? 1 : 0) << '\n';
    _rtcLastPersistHost = hostNow;
  } catch (...) {
    // Persistence is optional; do not break firmware simulation.
  }
}
