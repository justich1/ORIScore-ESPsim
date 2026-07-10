#pragma once
#include "Arduino.h"

#define WIFI_OFF 0
#define WIFI_STA 1
#define WIFI_AP 2
#define WIFI_AP_STA 3

#define WL_IDLE_STATUS 0
#define WL_NO_SSID_AVAIL 1
#define WL_SCAN_COMPLETED 2
#define WL_CONNECTED 3
#define WL_CONNECT_FAILED 4
#define WL_CONNECTION_LOST 5
#define WL_DISCONNECTED 6

// IPAddress is provided by arduino/include/Arduino.h

class FakeWiFiClass {
public:
  void mode(int m);

  int getMode() const {
    return currentMode;
  }

  bool softAP(const char* ssid, const char* pass = nullptr);

  bool softAPConfig(IPAddress local, IPAddress gateway, IPAddress subnet) {
    (void)local;
    (void)gateway;
    (void)subnet;
    return true;
  }

  IPAddress softAPIP();
  IPAddress localIP();

  bool softAPdisconnect(bool wifioff = false) {
    Serial.println(String("[SIM] WiFi.softAPdisconnect wifioff=") + String(wifioff ? 1 : 0));
    if (wifioff && currentMode == WIFI_AP) currentMode = WIFI_OFF;
    return true;
  }

  bool begin(const char* ssid, const char* pass = nullptr) {
    this->ssid = ssid ? ssid : "";
    (void)pass;
    currentMode = WIFI_STA;
    statusValue = WL_CONNECTED;
    Serial.println(String("[SIM] WiFi.begin ssid=") + this->ssid);
    return true;
  }

  int waitForConnectResult(uint32_t timeoutMs = 0) {
    (void)timeoutMs;
    Serial.println("[SIM] WiFi.waitForConnectResult -> WL_CONNECTED");
    statusValue = WL_CONNECTED;
    return WL_CONNECTED;
  }

  void disconnect(bool wifioff = false) {
    statusValue = WL_DISCONNECTED;
    if (wifioff) currentMode = WIFI_OFF;
    Serial.println(String("[SIM] WiFi.disconnect wifioff=") + String(wifioff ? 1 : 0));
  }

  void disconnect(bool wifioff, bool eraseap) {
    (void)eraseap;
    disconnect(wifioff);
  }

  int status() {
    return statusValue;
  }

  String SSID();

  String macAddress() {
    return "00:11:22:33:44:55";
  }

  String softAPmacAddress() {
    return "02:11:22:33:44:55";
  }

  int RSSI() {
    return -42;
  }

  void setSleep(bool enabled) {
    (void)enabled;
  }

  bool config(IPAddress local, IPAddress gateway, IPAddress subnet) {
    (void)local;
    (void)gateway;
    (void)subnet;
    return true;
  }

  bool hostname(const char* name) {
    (void)name;
    return true;
  }

private:
  int currentMode = WIFI_OFF;
  int statusValue = WL_DISCONNECTED;
  String ssid;
};

extern FakeWiFiClass WiFi;


class WiFiClient {
public:
  WiFiClient() = default;
  bool connect(const char* host, uint16_t port) {
    Serial.println(String("[SIM] WiFiClient.connect ") + String(host ? host : "") + ":" + String((int)port));
    _connected = true;
    return true;
  }
  bool connected() const { return _connected; }
  void stop() { _connected = false; }
  int available() { return 0; }
  int read() { return -1; }
  size_t write(uint8_t b) { (void)b; return 1; }
  size_t write(const uint8_t* data, size_t len) { (void)data; return len; }
  size_t print(const String& s) { Serial.print(String("[SIM] WiFiClient TX ") + s); return s.length(); }
  size_t print(const char* s) { return print(String(s ? s : "")); }
  size_t println(const String& s) { size_t n = print(s); Serial.println(""); return n + 2; }
  size_t println(const char* s) { return println(String(s ? s : "")); }
private:
  bool _connected = false;
};

class WiFiClientSecure : public WiFiClient {
public:
  void setInsecure() {}
  void setFingerprint(const char*) {}
};
