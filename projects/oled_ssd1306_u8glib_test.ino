/*
  ORIScore ESPsim - SSD1306/U8glib test
  Podpora simulátoru: SSD1306 I2C/SPI 128x64 a 128x32.

  Serial příkazy:
    HELP
    TEXT ahoj
    RELAY ON
    RELAY OFF
    RELAY TOGGLE
    CLEAR
    BOX
    STATUS

  Web:
    /
    /on
    /off
    /toggle
    /clear
    /box
    /status
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <U8glib.h>

#define RELAY_PIN D1
#define MODEM_SERIAL Serial

// I2C 128x64:
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

// I2C 128x32:
// U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);

// SPI 128x64, varianta CS/DC/RST:
// U8GLIB_SSD1306_128X64 u8g(D8, D2, D0);

// SPI 128x64, soft SPI SCK/MOSI/CS/DC/RST:
// U8GLIB_SSD1306_128X64 u8g(D5, D7, D8, D2, D0);

ESP8266WebServer server(80);
bool relayState = false;
String serialLine = "";
String displayLine = "READY";
int drawMode = 0;

void drawDisplay()
{
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_6x10);
    u8g.setColorIndex(1);
    u8g.drawStr(0, 10, "ORISCORE ESPSIM");
    u8g.drawLine(0, 13, 127, 13);

    u8g.drawStr(0, 27, displayLine.c_str());
    u8g.drawStr(0, 41, relayState ? "RELAY ON" : "RELAY OFF");

    if (drawMode == 1) {
      u8g.drawFrame(86, 20, 36, 24);
      u8g.drawBox(92, 26, 24, 12);
    }
    else {
      u8g.drawFrame(86, 20, 36, 24);
      u8g.drawLine(86, 20, 121, 43);
      u8g.drawLine(121, 20, 86, 43);
    }

    u8g.drawStr(0, 63, "TEXT/RELAY/BOX/CLEAR");
  } while (u8g.nextPage());
}

void setRelay(bool on)
{
  relayState = on;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  displayLine = relayState ? "RELAY ON" : "RELAY OFF";
  drawDisplay();
  Serial.print("[FW] RELAY ");
  Serial.println(relayState ? "ON" : "OFF");
}

String statusJson()
{
  String json = "{";
  json += "\"relay\":";
  json += relayState ? "true" : "false";
  json += ",\"display\":\"SSD1306 128x64\"";
  json += ",\"line\":\"";
  json += displayLine;
  json += "\"}";
  return json;
}

void sendPage()
{
  String h;
  h += "<!doctype html><html><head><meta charset='utf-8'>";
  h += "<title>OLED test</title>";
  h += "<style>body{font-family:Arial;margin:30px;background:#f4f4f4}.card{background:white;padding:20px;border-radius:12px;max-width:680px}a{display:inline-block;margin:6px;padding:10px 16px;background:#673ab7;color:white;text-decoration:none;border-radius:8px}pre{background:#111827;color:white;padding:12px;border-radius:8px}</style>";
  h += "</head><body><div class='card'>";
  h += "<h1>SSD1306 U8glib test</h1>";
  h += "<p>Relé: <b>";
  h += relayState ? "ON" : "OFF";
  h += "</b></p>";
  h += "<a href='/on'>Relé ON</a><a href='/off'>Relé OFF</a><a href='/toggle'>Toggle</a><a href='/box'>Box</a><a href='/clear'>Clear text</a><a href='/status'>JSON</a>";
  h += "<h3>Serial</h3><pre>HELP\nTEXT ahoj\nRELAY ON\nRELAY OFF\nRELAY TOGGLE\nBOX\nCLEAR\nSTATUS</pre>";
  h += "</div></body></html>";
  server.send(200, "text/html; charset=utf-8", h);
}

void handleRoot() { sendPage(); }
void handleOn() { setRelay(true); server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }
void handleOff() { setRelay(false); server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }
void handleToggle() { setRelay(!relayState); server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }
void handleClear() { displayLine = "CLEAR"; drawMode = 0; drawDisplay(); server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }
void handleBox() { drawMode = 1 - drawMode; displayLine = "BOX MODE"; drawDisplay(); server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); }
void handleStatus() { server.send(200, "application/json; charset=utf-8", statusJson()); }

void printHelp()
{
  Serial.println("[FW] Commands: HELP, TEXT <text>, RELAY ON, RELAY OFF, RELAY TOGGLE, BOX, CLEAR, STATUS");
}

void handleSerialCommand(String cmd)
{
  cmd.trim();
  if (!cmd.length()) return;

  String upper = cmd;
  upper.toUpperCase();

  Serial.print("[FW] RX: ");
  Serial.println(cmd);

  if (upper == "HELP") printHelp();
  else if (upper == "STATUS") Serial.println(statusJson());
  else if (upper == "RELAY ON") setRelay(true);
  else if (upper == "RELAY OFF") setRelay(false);
  else if (upper == "RELAY TOGGLE") setRelay(!relayState);
  else if (upper == "BOX") { drawMode = 1 - drawMode; displayLine = "BOX MODE"; drawDisplay(); }
  else if (upper == "CLEAR") { displayLine = "CLEAR"; drawMode = 0; drawDisplay(); }
  else if (upper.startsWith("TEXT ")) { displayLine = cmd.substring(5); drawDisplay(); }
  else Serial.println("[FW] Unknown command, type HELP");
}

void pollSerial()
{
  while (MODEM_SERIAL.available() > 0) {
    int ch = MODEM_SERIAL.read();
    if (ch < 0) return;
    char c = (char)ch;
    if (c == '\r') continue;
    if (c == '\n') {
      handleSerialCommand(serialLine);
      serialLine = "";
      continue;
    }
    serialLine += c;
    if (serialLine.length() > 120) serialLine = "";
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESPsim-OLED", "12345678");

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/toggle", handleToggle);
  server.on("/clear", handleClear);
  server.on("/box", handleBox);
  server.on("/status", handleStatus);
  server.begin();

  drawDisplay();
  Serial.println("[FW] SSD1306 U8glib test started. Type HELP.");
}

void loop()
{
  server.handleClient();
  pollSerial();
}
