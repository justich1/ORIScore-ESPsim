#include <Arduino.h>
#include <DHT.h>

// ESP8266 NodeMCU: D4 = GPIO2.
// ESP32 / ESP32-C3 / ESP32-S3 můžeš dát rovnou číslo GPIO, např. 4.
#define DHT_PIN D4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastPrint = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("ORIScore ESPsim - DHT test");
  Serial.println("Virtual HW nastav takto:");
  Serial.println("  Type: DHT22");
  Serial.println("  Pin: GPIO2   // nebo D4 na ESP8266 NodeMCU");
  Serial.println("  Value: 23.5  // teplota C");
  Serial.println("  Humidity: 55 // vlhkost %");
  Serial.println("  Connected: true");
  Serial.println();

  dht.begin();
}

void loop() {
  if (millis() - lastPrint < 2000) {
    return;
  }
  lastPrint = millis();

  float humidity = dht.readHumidity(true);
  float temperature = dht.readTemperature(false, true);
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT: chyba cteni / cidlo neni pripojene ve Virtual HW");
    return;
  }

  Serial.print("DHT teplota: ");
  Serial.print(temperature, 1);
  Serial.print(" C, vlhkost: ");
  Serial.print(humidity, 1);
  Serial.print(" %, heat-index: ");
  Serial.print(heatIndex, 1);
  Serial.println(" C");

  Serial.print("RAMSTATE TOTAL=");
  Serial.print(ESP.getFreeHeap() + (320000 - ESP.getFreeHeap()));
  Serial.print(" FREE=");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" MAX=");
  Serial.println(ESP.getMaxFreeBlockSize());
}
