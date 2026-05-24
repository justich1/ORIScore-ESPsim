#include "ESP8266WiFi.h"
FakeWiFiClass WiFi;
void FakeWiFiClass::mode(int m) { currentMode = m; }
bool FakeWiFiClass::softAP(const char* s, const char*) { ssid = s ? s : ""; currentMode = WIFI_AP; statusValue = WL_CONNECTED; return true; }
IPAddress FakeWiFiClass::softAPIP() { return IPAddress(192,168,4,1); }
IPAddress FakeWiFiClass::localIP() { return IPAddress(127,0,0,1); }
String FakeWiFiClass::SSID() { return ssid; }
