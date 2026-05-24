/*
  ORIScore ESPsim - Arduino core phase 1 test
  -------------------------------------------
  Testuje základní Arduino core stub:
    - Serial / Print / Stream základ
    - String operace
    - číselné výpisy DEC/HEX/BIN
    - IPAddress + Print
    - millis/delay/yield
    - digitalWrite/digitalRead/pinMode
    - random/randomSeed
    - map/constrain/min/max

  Nic neposílá periodicky samo.
  Vypíše jen start v setup() a pak reaguje na ruční příkazy ze serial konzole.

  Příkazy:
    HELP
    STATUS
    PRINT
    STRING
    IP
    PIN ON
    PIN OFF
    PIN READ
    RANDOM
    MATH 123
    ECHO ahoj světe
*/

#include <Arduino.h>

#define TEST_LED_PIN D1

String rxLine;
bool ledState = false;
unsigned long bootMs = 0;

static String trimCopy(String s) {
  s.trim();
  return s;
}

static String upperCopy(String s) {
  s.toUpperCase();
  return s;
}

static void setLed(bool on) {
  ledState = on;
  digitalWrite(TEST_LED_PIN, ledState ? HIGH : LOW);

  Serial.print("[CORETEST] PIN ");
  Serial.print(TEST_LED_PIN);
  Serial.print(" = ");
  Serial.println(ledState ? "HIGH" : "LOW");
}

static void printHelp() {
  Serial.println("[CORETEST] Commands:");
  Serial.println("[CORETEST]   HELP");
  Serial.println("[CORETEST]   STATUS");
  Serial.println("[CORETEST]   PRINT");
  Serial.println("[CORETEST]   STRING");
  Serial.println("[CORETEST]   IP");
  Serial.println("[CORETEST]   PIN ON");
  Serial.println("[CORETEST]   PIN OFF");
  Serial.println("[CORETEST]   PIN READ");
  Serial.println("[CORETEST]   RANDOM");
  Serial.println("[CORETEST]   MATH 123");
  Serial.println("[CORETEST]   ECHO ahoj");
}

static void testPrint() {
  Serial.println("[CORETEST] --- Print test ---");

  Serial.print("[CORETEST] int DEC: ");
  Serial.println(1234);

  Serial.print("[CORETEST] int HEX: ");
  Serial.println(1234, HEX);

  Serial.print("[CORETEST] int BIN: ");
  Serial.println(10, BIN);

  Serial.print("[CORETEST] float default: ");
  Serial.println(12.3456);

  Serial.print("[CORETEST] float 3 digits: ");
  Serial.println(12.3456, 3);

  Serial.print("[CORETEST] concat: ");
  Serial.println(String("abc") + String(123) + String("_xyz"));
}

static void testString() {
  Serial.println("[CORETEST] --- String test ---");

  String s = "  venkovniCidlo_Temperature=22.75  ";
  Serial.print("[CORETEST] raw='");
  Serial.print(s);
  Serial.println("'");

  s.trim();
  Serial.print("[CORETEST] trim='");
  Serial.print(s);
  Serial.println("'");

  Serial.print("[CORETEST] length=");
  Serial.println((int)s.length());

  Serial.print("[CORETEST] indexOf '=' = ");
  Serial.println(s.indexOf('='));

  String name = s.substring(0, s.indexOf('='));
  String val = s.substring(s.indexOf('=') + 1);

  Serial.print("[CORETEST] name=");
  Serial.println(name);

  Serial.print("[CORETEST] value=");
  Serial.println(val.toFloat());

  name.replace("_", ".");
  Serial.print("[CORETEST] replaced=");
  Serial.println(name);

  name.toUpperCase();
  Serial.print("[CORETEST] upper=");
  Serial.println(name);
}

static void testIp() {
  Serial.println("[CORETEST] --- IPAddress test ---");

  IPAddress ip(192, 168, 4, 123);
  IPAddress gw(192, 168, 4, 1);

  Serial.print("[CORETEST] ip=");
  Serial.println(ip);

  Serial.print("[CORETEST] gateway=");
  Serial.println(gw);

  Serial.print("[CORETEST] ip[0]=");
  Serial.println((int)ip[0]);

  Serial.print("[CORETEST] ip string=");
  Serial.println(ip.toString());
}

static void testMath(long input) {
  Serial.println("[CORETEST] --- Math helpers test ---");

  long constrained = constrain(input, 0, 100);
  long mapped = map(constrained, 0, 100, 0, 255);

  Serial.print("[CORETEST] input=");
  Serial.println(input);

  Serial.print("[CORETEST] constrain 0..100=");
  Serial.println(constrained);

  Serial.print("[CORETEST] map 0..100 -> 0..255=");
  Serial.println(mapped);

  Serial.print("[CORETEST] min(input,50)=");
  Serial.println(min(input, 50));

  Serial.print("[CORETEST] max(input,50)=");
  Serial.println(max(input, 50));
}

static void printStatus() {
  Serial.println("[CORETEST] --- Status ---");

  Serial.print("[CORETEST] millis=");
  Serial.println(millis());

  Serial.print("[CORETEST] since boot=");
  Serial.println(millis() - bootMs);

  Serial.print("[CORETEST] LED state=");
  Serial.println(ledState ? "ON" : "OFF");

  Serial.print("[CORETEST] digitalRead=");
  Serial.println(digitalRead(TEST_LED_PIN));

  Serial.print("[CORETEST] free heap=");
  Serial.println(ESP.getFreeHeap());

  Serial.print("[CORETEST] reset reason=");
  Serial.println(ESP.getResetReason());
}

static void handleCommand(String line) {
  line.trim();
  if (!line.length()) return;

  Serial.print("[CORETEST] RX: ");
  Serial.println(line);

  String up = line;
  up.toUpperCase();

  if (up == "HELP") {
    printHelp();
  }
  else if (up == "STATUS") {
    printStatus();
  }
  else if (up == "PRINT") {
    testPrint();
  }
  else if (up == "STRING") {
    testString();
  }
  else if (up == "IP") {
    testIp();
  }
  else if (up == "PIN ON") {
    setLed(true);
  }
  else if (up == "PIN OFF") {
    setLed(false);
  }
  else if (up == "PIN READ") {
    Serial.print("[CORETEST] PIN READ ");
    Serial.println(digitalRead(TEST_LED_PIN));
  }
  else if (up == "RANDOM") {
    Serial.print("[CORETEST] random(1000)=");
    Serial.println(random(1000));
    Serial.print("[CORETEST] random(10,20)=");
    Serial.println(random(10, 20));
  }
  else if (up.startsWith("MATH ")) {
    String arg = line.substring(5);
    arg.trim();
    testMath(arg.toInt());
  }
  else if (up.startsWith("ECHO ")) {
    Serial.print("[CORETEST] ECHO: ");
    Serial.println(line.substring(5));
  }
  else {
    Serial.print("[CORETEST] Unknown command: ");
    Serial.println(line);
    Serial.println("[CORETEST] Type HELP");
  }
}

static void pollSerial() {
  while (Serial.available() > 0) {
    int ch = Serial.read();
    if (ch < 0) return;

    char c = (char)ch;

    if (c == '\r') continue;

    if (c == '\n') {
      handleCommand(rxLine);
      rxLine = "";
      continue;
    }

    rxLine += c;

    if (rxLine.length() > 180) {
      Serial.println("[CORETEST] RX line too long, clearing");
      rxLine = "";
    }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(12345);

  pinMode(TEST_LED_PIN, OUTPUT);
  setLed(false);

  bootMs = millis();

  Serial.println();
  Serial.println("[CORETEST] Arduino core phase 1 test started");
  Serial.println("[CORETEST] Nothing is sent periodically. Type HELP.");
}

void loop() {
  pollSerial();
  yield();
}
