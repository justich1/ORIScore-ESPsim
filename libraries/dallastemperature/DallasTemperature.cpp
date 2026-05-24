#include "DallasTemperature.h"

#include <cstdio>
#include <cctype>

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return -1;
}

static String normalizeDsAddress(String in) {
  in.trim();
  in.toUpperCase();

  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) out += c;
  }

  // Linux/w1 formát DS18B20 může být bez CRC bajtu: 28-000000000001 => 14 HEX.
  // Simulátor používá Dallas DeviceAddress[8], proto chybějící CRC bajt doplníme jako 00.
  if (out.length() == 14 && out.startsWith("28")) out += "00";

  return out;
}

static bool hexToAddr(const String& sIn, DeviceAddress out) {
  String s = normalizeDsAddress(sIn);
  if (s.length() != 16) return false;

  for (int i = 0; i < 8; i++) {
    int hi = hexNibble(s[i * 2]);
    int lo = hexNibble(s[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    out[i] = (uint8_t)((hi << 4) | lo);
  }

  return true;
}

static String addrToHex(const DeviceAddress addr) {
  char buf[17];
  for (int i = 0; i < 8; i++) std::sprintf(buf + i * 2, "%02X", addr[i]);
  buf[16] = 0;
  return String(buf);
}

// Zpětná kompatibilita se starými SENSOR příkazy/testy.
// Nově ale ukládá do společného virtual-HW skladu v Arduino.cpp, aby HW SET a DallasTemperature viděly stejná data.
void simDs18b20SetTemp(const String& hexAddress, float temp, bool connected) {
  simVirtualHwSet(String("DS18B20"), String("ds18b20"), String(""), normalizeDsAddress(hexAddress),
                  String(temp, 2), String(""), String(""), connected);
}

void simDs18b20Remove(const String& hexAddress) {
  (void)hexAddress;
  // Současný HW bridge posílá po změně celý seznam: HW CLEAR + HW SET...
  // Selektivní remove tady záměrně neděláme, aby se nerozbíjely ostatní typy virtual HW.
}

void simDs18b20Clear() {
  simVirtualHwClear();
}

DallasTemperature::DallasTemperature(OneWire* o) : ow(o) {}

void DallasTemperature::begin() {}

void DallasTemperature::requestTemperatures() {
  // Hodnoty dodává ORIScore ESPsim přes HW SET.
}

void DallasTemperature::setWaitForConversion(bool wait) {
  (void)wait;
}

int DallasTemperature::getDeviceCount() {
  return simVirtualHwCount(String("DS18B20"));
}

bool DallasTemperature::getAddress(DeviceAddress addr, int index) {
  SimVirtualDevice d;
  if (!simVirtualHwGetByTypeIndex(String("DS18B20"), index, &d)) return false;
  return hexToAddr(d.address, addr);
}

float DallasTemperature::getTempC(const DeviceAddress addr) {
  String h = addrToHex(addr);

  SimVirtualDevice d;
  if (!simVirtualHwFindDs18b20ByAddress(h, &d)) return DEVICE_DISCONNECTED_C;
  return d.connected ? d.value.toFloat() : DEVICE_DISCONNECTED_C;
}

float DallasTemperature::getTempCByIndex(int index) {
  DeviceAddress addr;
  if (!getAddress(addr, index)) return DEVICE_DISCONNECTED_C;
  return getTempC(addr);
}

bool simDs18b20GetRawAddressAt(int index, uint8_t* addr) {
  if (!addr) return false;

  SimVirtualDevice d;
  if (!simVirtualHwGetByTypeIndex(String("DS18B20"), index, &d)) return false;

  DeviceAddress tmp;
  if (!hexToAddr(d.address, tmp)) return false;

  for (int i = 0; i < 8; i++) addr[i] = tmp[i];
  return true;
}

bool DallasTemperature::validAddress(const DeviceAddress addr) {
  if (!addr) return false;
  for (int i = 0; i < 8; i++) {
    if (addr[i] != 0) return true;
  }
  return false;
}

void DallasTemperature::setFakeDevice(const String& hexAddress, float temp) {
  simDs18b20SetTemp(hexAddress, temp, true);
}
