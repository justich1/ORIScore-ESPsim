# ORIScore ESPsim

**ORIScore ESPsim** je Windows simulátor pro vývoj a ladění Arduino/ESP firmwaru bez nutnosti neustále nahrávat kód do fyzického ESP modulu.

Projekt je určený hlavně pro rychlé testování webového rozhraní firmwaru, OLED výstupu, sériové komunikace, GPIO, virtuálních senzorů, modemů a základního chování firmware smyčky `setup()` / `loop()`.

> ESPsim není plný binární emulátor ESP čipu. Firmware se nepouští jako skutečný ESP8266/ESP32 binární obraz, ale překládá se proti simulační vrstvě a stub/fake knihovnám pro Windows/MSVC.

---

## K čemu je to dobré

ORIScore ESPsim vznikl pro rychlejší vývoj firmware pro ESP zařízení, kde je zdlouhavé pořád dokola kompilovat, nahrávat a testovat na fyzickém hardware.

Typické použití:

* ladění webového rozhraní firmwaru,
* testování OLED výpisů,
* kontrola sériové komunikace,
* testování AT komunikace s modemem,
* simulace GPIO vstupů a výstupů,
* simulace virtuálních senzorů,
* rychlé testování logiky bez připojeného ESP,
* vývoj firmware před zapojením reálného hardware.

Simulátor je vhodný hlavně pro projekty, kde je hodně aplikační logiky, webu, stavů, konfigurace a komunikace. Není určený pro přesnou emulaci Wi-Fi rádia, časování CPU, periferií nebo nízkoúrovňového ESP SDK.

---

## Hlavní funkce

* Spuštění Arduino/ESP `.ino` projektu ve Windows simulátoru.
* Simulace základních Arduino funkcí jako `setup()`, `loop()`, `millis()`, `delay()`.
* Fake/stub knihovny pro běžné ESP/Arduino knihovny.
* Virtuální sériová konzole.
* Vstup pro simulaci odpovědí modemu.
* Náhled firmware logu.
* Build log s chybami překladu.
* Lokální fake webserver firmwaru.
* GPIO panel s klikacími vstupy/výstupy.
* Podpora board profilů přes JSON.
* Simulace virtuálních senzorů přes JSON.
* Nastavitelná rychlost běhu simulace.
* Restart firmwaru bez restartu celé aplikace.
* Náhled RAM/heap stavu podle dat zaslaných firmwarem.

---

## Co ESPsim není

ESPsim není:

* náhrada za reálný test na ESP,
* přesný ESP8266/ESP32 emulátor,
* emulátor Wi-Fi rádia,
* emulátor LoRaWAN stacku na úrovni rádia,
* emulátor přesného časování CPU,
* prostředí plně kompatibilní s Arduino GCC toolchainem.

Některé knihovny je potřeba doplnit jako simulační stuby. Kód se ve Windows překládá přes MSVC, takže některé věci, které projdou v Arduino IDE, mohou vyžadovat drobnou kompatibilní úpravu.

---

## Požadavky

Pro spuštění hotové aplikace:

* Windows 10 nebo Windows 11,
* rozbalený ORIScore ESPsim,
* firmware projekt ve formátu `.ino`,
* složka se simulačními knihovnami.

Pro build ze zdrojových kódů:

* Visual Studio nebo Visual Studio Build Tools,
* workload pro C++ desktop build,
* .NET Desktop podle použité verze projektu,
* Git, pokud chceš projekt stahovat z repozitáře.

---

## Rychlý start

1. Spusť `ORIScore ESPsim.exe`.
2. Vyber firmware `.ino` soubor.
3. Vyber board profil, například ESP8266 NodeMCU nebo ESP32-S3.
4. Zkontroluj cestu k simulačním knihovnám.
5. Klikni na build/spuštění firmware.
6. Sleduj výstup v konzoli, firmware logu, GPIO panelu a web náhledu.

Pokud firmware používá webserver, simulátor jej zpřístupní lokálně, typicky například na:

```text
http://localhost:18088/
```

Přesná adresa a port se mohou lišit podle nastavení simulátoru.

---

## Doporučená struktura projektu

```text
ORIScore-ESPsim/
├── ORIScore ESPsim.exe
├── README.md
├── config/
│   ├── boards/
│   │   ├── esp8266_nodemcu_v3.json
│   │   ├── esp32_c3_generic.json
│   │   └── esp32_s3_generic.json
│   └── hardware/
│       ├── aht20_i2c.json
│       ├── ds18b20_onewire.json
│       └── modem_at.json
├── libraries/
│   ├── Arduino.h
│   ├── ESP8266WiFi.h
│   ├── ESP8266WebServer.h
│   ├── LittleFS.h
│   ├── FS.h
│   ├── OneWire.h
│   ├── DallasTemperature.h
│   ├── Updater.h
│   └── ArduinoJson.h
├── examples/
│   ├── SmokeTest.ino
│   └── WebServerTest.ino
└── Builds/
    └── Current/
```

Cílem je držet simulátor přenosný vedle EXE. Knihovny a profily je možné verzovat samostatně a podle potřeby aktualizovat z repozitáře.

---

## Simulační knihovny

Simulátor používá fake/stub knihovny, které nahrazují vybrané Arduino/ESP knihovny tak, aby šel firmware přeložit a spustit pod Windows.

Příklady podporovaných nebo plánovaných stubů:

* `Arduino.h`
* `ESP8266WiFi.h`
* `ESP8266WebServer.h`
* `LittleFS.h`
* `FS.h`
* `OneWire.h`
* `DallasTemperature.h`
* `Updater.h`
* `ArduinoJson.h`

Stub knihovna nemusí implementovat úplně všechno. Cílem je pokrýt běžné použití ve firmware a postupně doplňovat chybějící funkce podle reálných projektů.

---

## Board profily

Board profil popisuje simulovanou desku, její piny, dostupnou RAM a základní vlastnosti.

Příklad zjednodušeného board profilu:

```json
{
  "id": "esp32_s3_generic",
  "name": "ESP32-S3 Generic",
  "family": "esp32",
  "ram_total": 327680,
  "pins": [
    { "name": "GPIO1", "mode": "input_output" },
    { "name": "GPIO2", "mode": "input_output" },
    { "name": "GPIO21", "mode": "input_output" },
    { "name": "GPIO22", "mode": "input_output" }
  ]
}
```

Board JSON určuje hlavně kapacitu a možnosti simulované desky. Aktuální zatížení RAM neposílá board profil, ale samotný firmware za běhu.

---

## Virtuální hardware

Virtuální zařízení se definují přes JSON soubory. Díky tomu lze testovat firmware bez fyzicky připojeného senzoru, relé nebo modemu.

Příklad virtuálního I2C senzoru AHT20:

```json
{
  "id": "aht20_i2c",
  "name": "AHT20 I2C",
  "version": "1.0.0",
  "description": "I2C senzor teploty a vlhkosti.",
  "type": "i2c",
  "devices": [
    {
      "type": "AHT20",
      "name": "aht20",
      "address": "0x38",
      "sda": "GPIO21",
      "scl": "GPIO22",
      "temperature": 22.0,
      "humidity": 45.0,
      "connected": true,
      "noise": 0.1
    }
  ]
}
```

Takto lze postupně doplňovat další typy hardware:

* I2C senzory,
* 1-Wire senzory,
* digitální vstupy,
* relé výstupy,
* tlačítka,
* modem/AT zařízení,
* OLED displej,
* vlastní virtuální periferie.

---

## GPIO panel

GPIO panel zobrazuje piny zvoleného board profilu.

Podle typu pinu můžeš:

* sledovat stav výstupu,
* měnit stav vstupu kliknutím,
* simulovat tlačítka,
* kontrolovat, jestli firmware nastavuje správné piny,
* ladit logiku bez fyzického zapojení.

---

## Konzole a modem

Simulátor obsahuje konzoli pro sériový výstup firmwaru a vstupní pole pro simulaci odpovědí modemu.

To je užitečné například pro firmware, který komunikuje přes AT příkazy:

```text
APP: MODEM help
SERIAL < help
ESP > [CORETEST] RX: help
ESP > [CORETEST] Commands:
ESP > [CORETEST]   HELP
```

Díky tomu lze testovat parsování odpovědí, retry logiku a chování při výpadku modemu bez reálného modulu.

---

## Webserver firmwaru

Pokud firmware používá webové rozhraní, ESPsim jej zpřístupní přes lokální webserver.

Typické využití:

* kontrola HTML výstupu,
* ladění jednoduchých formulářů,
* testování konfigurace,
* kontrola responzivity,
* rychlé opravy bez nahrávání do ESP.

Díky tomu je možné rychle ladit firmware web, který by se jinak testoval až na fyzickém zařízení.

---

## RAM / heap náhled

Board profil definuje celkovou kapacitu RAM, ale skutečný stav RAM posílá firmware za běhu.

Doporučený formát zprávy:

```text
RAMSTATE TOTAL=327680 FREE=245120 MAX=180000
```

Simulátor z toho může zobrazit:

* celkovou RAM,
* volnou RAM,
* obsazenou RAM,
* největší volný blok,
* orientační fragmentaci heapu.

RAM panel tedy nemá jen kreslit náhodnou hodnotu z JSONu, ale ukazovat runtime stav firmwaru.

---

## Build log

Build log zobrazuje výstup překladu firmware. Slouží hlavně pro rychlé hledání chyb kompatibility mezi Arduino/ESP světem a Windows/MSVC buildem.

Typické chyby:

* chybějící stub knihovna,
* funkce dostupná v Arduino GCC, ale ne v MSVC,
* nekompatibilní include,
* rozdíly mezi ESP8266 a ESP32 knihovnami,
* chybějící definice typu nebo konstanty.

Příklad:

```text
GeneratedFirmware.cpp(15): fatal error C1083: Nejde otevřít soubor zahrnout: ESP8266HTTPClient.h: No such file or directory
```

Řešení je obvykle doplnit odpovídající stub knihovnu nebo upravit kompatibilní vrstvu.

---

## Příklad testovacího firmware

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  Serial.println("ORIScore ESPsim smoke test");
}

void loop() {
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);

  Serial.println("RAMSTATE TOTAL=327680 FREE=245120 MAX=180000");
}
```

Tento firmware otestuje:

* spuštění `setup()`,
* periodické volání `loop()`,
* sériový výstup,
* GPIO výstup,
* základní RAM stav.

---

## Známé limity

* Ne všechny Arduino/ESP knihovny jsou dostupné jako stub.
* MSVC není Arduino GCC, takže některý firmware může vyžadovat úpravy.
* Přesné časování hardwaru není garantováno.
* Síťová komunikace je simulovaná jen na úrovni, kterou ESPsim implementuje.
* LoRaWAN, Wi-Fi a modemové chování není plná fyzická simulace.
* Chování na reálném ESP je nutné před nasazením ověřit na skutečném hardware.

---

## Doporučený workflow

1. Napiš nebo uprav firmware.
2. Spusť jej v ESPsim.
3. Ověř web, konzoli, GPIO a virtuální senzory.
4. Oprav chyby v logice nebo HTML.
5. Teprve potom nahraj firmware do reálného ESP.
6. Na hardware ověř věci závislé na reálném čase, Wi-Fi, rádiu a periferiích.

Tím se výrazně zkrátí vývojový cyklus, protože většina aplikačních chyb se dá odladit bez připojené desky.

---

## Roadmap

Plánované nebo vhodné rozšíření:

* knihovní prohlížeč a stahování jednotlivých `.h` / `.cpp` stubů,
* více board profilů,
* rozšíření ESP32-S3 kompatibility,
* lepší simulace LittleFS,
* detailnější OLED náhled,
* editor virtuálního hardware,
* historie buildů,
* validace JSON profilů,
* přehled poslední chyby a posledního spuštění,
* timeouty a lepší runtime stavy,
* detailnější RAM/heap fragmentace,
* ukázkové projekty.

---

## Přispívání

Příspěvky jsou vítané hlavně v těchto oblastech:

* doplnění stub knihoven,
* oprava kompatibility s různými Arduino/ESP projekty,
* nové board profily,
* nové virtuální senzory,
* ukázkové `.ino` projekty,
* opravy dokumentace,
* testování na různých Windows konfiguracích.

Při hlášení chyby je ideální přiložit:

* použitý `.ino` soubor nebo minimální ukázku,
* build log,
* použitý board profil,
* seznam použitých knihoven,
* stručný popis očekávaného a skutečného chování.

---

## Licence

MIT

Doporučené možnosti:

* MIT pro otevřené komunitní použití,
* GPL, pokud chceš vynutit zveřejnění odvozených úprav,
* vlastní licence, pokud chceš projekt držet částečně omezený.

---

## Stav projektu

ORIScore ESPsim je vývojový nástroj vzniklý z reálné potřeby ladit firmware rychleji než přes fyzické ESP zařízení.

Projekt je praktický hlavně pro vývojáře, kteří řeší firmware s webovým rozhraním, konfigurací, senzory, relé, sériovou komunikací a větší aplikační logikou.

Před produkčním nasazením firmware je stále nutné vše ověřit na reálném hardware.
