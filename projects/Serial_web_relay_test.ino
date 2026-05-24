/*
  ORIScore ESPsim - jednoduchý test web + relé + serial konzole
  -------------------------------------------------------------
  Nic neposílá samo do serialu.
  Firmware reaguje jen na příkazy, které pošleš ručně do konzole/modemu.

  Serial příkazy:
    HELP
    STATUS
    RELAY ON
    RELAY OFF
    RELAY TOGGLE
    PING
    OK
    ERROR

  Web:
    /
    /on
    /off
    /toggle
    /status
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define RELAY_PIN D1

// Tady se přepíná, přes co firmware čte konzoli/modem.
// Pro ESPsim test nech Serial:
#define MODEM_SERIAL Serial

ESP8266WebServer server(80);

bool relayState = false;
String serialLine = "";

void setRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  Serial.print("[FW] RELAY ");
  Serial.println(relayState ? "ON" : "OFF");
}

String statusJson() {
  String json = "{";
  json += "\"relay\":";
  json += relayState ? "true" : "false";
  json += ",\"relay_pin\":\"D1\"";
  json += ",\"millis\":";
  json += String(millis());
  json += "}";
  return json;
}

void sendPage() {
  String h;
  h += "<!doctype html><html><head><meta charset='utf-8'>";
  h += "<title>ESPsim Relay Test</title>";
  h += "<style>";
  h += "body{font-family:Arial;margin:30px;background:#f4f4f4}";
  h += ".card{background:white;padding:20px;border-radius:12px;max-width:600px}";
  h += "a,button{display:inline-block;margin:6px;padding:10px 16px;background:#673ab7;color:white;text-decoration:none;border-radius:8px;border:0}";
  h += "pre{background:#111827;color:#fff;padding:12px;border-radius:8px}";
  h += "</style></head><body><div class='card'>";
  h += "<h1>ESPsim Relay Test</h1>";
  h += "<p>Relé: <b>";
  h += relayState ? "ON" : "OFF";
  h += "</b></p>";
  h += "<a href='/on'>Zapnout</a>";
  h += "<a href='/off'>Vypnout</a>";
  h += "<a href='/toggle'>Přepnout</a>";
  h += "<a href='/status'>JSON stav</a>";
  h += "<h3>Serial příkazy</h3>";
  h += "<pre>HELP\nSTATUS\nRELAY ON\nRELAY OFF\nRELAY TOGGLE\nPING\nOK\nERROR</pre>";
  h += "</div></body></html>";

  server.send(200, "text/html; charset=utf-8", h);
}

void handleRoot() {
  sendPage();
}

void handleOn() {
  setRelay(true);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleOff() {
  setRelay(false);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleToggle() {
  setRelay(!relayState);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  server.send(200, "application/json; charset=utf-8", statusJson());
}

void printHelp() {
  Serial.println("[FW] Commands:");
  Serial.println("[FW]   HELP");
  Serial.println("[FW]   STATUS");
  Serial.println("[FW]   RELAY ON");
  Serial.println("[FW]   RELAY OFF");
  Serial.println("[FW]   RELAY TOGGLE");
  Serial.println("[FW]   PING");
  Serial.println("[FW]   OK");
  Serial.println("[FW]   ERROR");
}

void handleSerialCommand(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;

  String upper = cmd;
  upper.toUpperCase();

  Serial.print("[FW] RX: ");
  Serial.println(cmd);

  if (upper == "HELP") {
    printHelp();
  }
  else if (upper == "STATUS") {
    Serial.print("[FW] STATUS ");
    Serial.println(statusJson());
  }
  else if (upper == "RELAY ON") {
    setRelay(true);
  }
  else if (upper == "RELAY OFF") {
    setRelay(false);
  }
  else if (upper == "RELAY TOGGLE") {
    setRelay(!relayState);
  }
  else if (upper == "PING") {
    Serial.println("[FW] PONG");
  }
  else if (upper == "OK") {
    Serial.println("[FW] OK received");
  }
  else if (upper == "ERROR") {
    Serial.println("[FW] ERROR received");
  }
  else {
    Serial.print("[FW] Unknown command: ");
    Serial.println(cmd);
    Serial.println("[FW] Type HELP");
  }
}

void pollSerial() {
  while (MODEM_SERIAL.available() > 0) {
    int ch = MODEM_SERIAL.read();
    if (ch < 0) return;

    char c = (char)ch;

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      handleSerialCommand(serialLine);
      serialLine = "";
      continue;
    }

    serialLine += c;

    if (serialLine.length() > 120) {
      Serial.println("[FW] Serial line too long, clearing");
      serialLine = "";
    }
  }
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);

  Serial.begin(9600);
  Serial.println();
  Serial.println("[FW] ESPsim relay serial test started");
  Serial.println("[FW] Nothing is sent automatically. Type HELP.");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESPsim-Test", "12345678");

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("[FW] HTTP server started");
}

void loop() {
  server.handleClient();
  pollSerial();
}
