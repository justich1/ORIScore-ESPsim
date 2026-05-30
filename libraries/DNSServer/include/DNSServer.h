#pragma once

/*
  DNSServer.h - stub knihovna pro ORIS ESPsim

  Účel:
  - umožní zkompilovat ESP8266/ESP32 sketch, který používá DNSServer
  - v simulátoru DNS požadavky reálně neřeší
  - vhodné hlavně pro captive portal kód typu:

      DNSServer dnsServer;
      dnsServer.start(53, "*", WiFi.softAPIP());
      dnsServer.processNextRequest();

  Poznámka:
  - Na reálném ESP používej originální DNSServer knihovnu z ESP8266/ESP32 core.
*/

#include <Arduino.h>

#ifndef DNS_REPLY_CODE_DEFINED
#define DNS_REPLY_CODE_DEFINED

enum DNSReplyCode
{
    NoError = 0,
    FormError = 1,
    ServerFailure = 2,
    NonExistentDomain = 3,
    NotImplemented = 4,
    Refused = 5,
    YXDomain = 6,
    YXRRSet = 7,
    NXRRSet = 8
};

#endif

class DNSServer
{
public:
    DNSServer()
    {
    }

    ~DNSServer()
    {
        stop();
    }

    bool start(const uint16_t& port, const String& domainName, const IPAddress& resolvedIP)
    {
        _port = port;
        _domainName = domainName;
        _resolvedIP = resolvedIP;
        _started = true;

#ifdef SERIAL_DEBUG
        Serial.print("[DNSServer SIM] start port=");
        Serial.print(_port);
        Serial.print(" domain=");
        Serial.print(_domainName);
        Serial.print(" ip=");
        Serial.println(_resolvedIP);
#endif

        return true;
    }

    bool start(const uint16_t& port, const char* domainName, const IPAddress& resolvedIP)
    {
        return start(port, String(domainName), resolvedIP);
    }

    void stop()
    {
        _started = false;
    }

    void processNextRequest()
    {
        // V simulátoru není potřeba nic zpracovávat.
        // Na reálném ESP originální knihovna odpovídá na DNS dotazy.
    }

    void setErrorReplyCode(const DNSReplyCode& replyCode)
    {
        _errorReplyCode = replyCode;
    }

    void setTTL(const uint32_t& ttl)
    {
        _ttl = ttl;
    }

    bool isRunning() const
    {
        return _started;
    }

private:
    bool _started = false;
    uint16_t _port = 53;
    String _domainName = "*";
    IPAddress _resolvedIP;
    DNSReplyCode _errorReplyCode = NoError;
    uint32_t _ttl = 60;
};