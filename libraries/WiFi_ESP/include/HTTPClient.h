#pragma once

#include "Arduino.h"
#include "WiFi.h"

#ifndef HTTP_CODE_OK
#define HTTP_CODE_OK 200
#endif
#ifndef HTTP_CODE_CREATED
#define HTTP_CODE_CREATED 201
#endif
#ifndef HTTP_CODE_NO_CONTENT
#define HTTP_CODE_NO_CONTENT 204
#endif
#ifndef HTTP_CODE_BAD_REQUEST
#define HTTP_CODE_BAD_REQUEST 400
#endif
#ifndef HTTP_CODE_NOT_FOUND
#define HTTP_CODE_NOT_FOUND 404
#endif
#ifndef HTTP_CODE_INTERNAL_SERVER_ERROR
#define HTTP_CODE_INTERNAL_SERVER_ERROR 500
#endif

class HTTPClient {
public:
  HTTPClient() = default;

  bool begin(const String& url) {
    _url = url;
    _active = true;
    Serial.println(String("[SIM] HTTPClient.begin ") + _url);
    return true;
  }

  bool begin(const char* url) {
    return begin(String(url ? url : ""));
  }

  bool begin(WiFiClient& client, const String& url) {
    (void)client;
    return begin(url);
  }

  bool begin(WiFiClient& client, const char* url) {
    (void)client;
    return begin(String(url ? url : ""));
  }

  void end() {
    Serial.println("[SIM] HTTPClient.end");
    _active = false;
  }

  void addHeader(const String& name, const String& value, bool first = false, bool replace = true) {
    (void)first;
    (void)replace;
    _headers[name] = value;
    Serial.println(String("[SIM] HTTP header ") + name + ": " + value);
  }

  void addHeader(const char* name, const char* value) {
    addHeader(String(name ? name : ""), String(value ? value : ""));
  }

  void setAuthorization(const char* user, const char* password) {
    _authUser = user ? user : "";
    _authPassword = password ? password : "";
    Serial.println("[SIM] HTTPClient.setAuthorization");
  }

  void setAuthorization(const String& user, const String& password) {
    setAuthorization(user.c_str(), password.c_str());
  }

  int GET() {
    _lastMethod = "GET";
    _lastPayload = "";
    _lastCode = 200;
    _lastResponse = "{}";
    Serial.println(String("[SIM] HTTP GET ") + _url);
    return _lastCode;
  }

  int POST(const String& payload) {
    _lastMethod = "POST";
    _lastPayload = payload;
    _lastCode = 200;

    Serial.println(String("[SIM] HTTP POST ") + _url);
    Serial.println(String("[SIM] HTTP payload ") + payload);

    // Helpful fake responses for common ORIS/thermostat examples.
    if (_url.indexOf("api_register_device") >= 0) {
      _lastResponse = "{\"token\":\"SIM_TOKEN_123456\"}";
    }
    else if (_url.indexOf("api_thermostat") >= 0) {
      _lastResponse =
        "{\"action\":\"noop\","
        "\"updated_at\":\"1970-01-01 00:00:00\"}";
    }
    else {
      _lastResponse = "{}";
    }

    return _lastCode;
  }

  int POST(const char* payload) {
    return POST(String(payload ? payload : ""));
  }

  int POST(uint8_t* payload, size_t size) {
    String s;
    for (size_t i = 0; i < size; i++) s += (char)payload[i];
    return POST(s);
  }

  int PUT(const String& payload) {
    _lastMethod = "PUT";
    _lastPayload = payload;
    _lastCode = 200;
    _lastResponse = "{}";
    Serial.println(String("[SIM] HTTP PUT ") + _url);
    Serial.println(String("[SIM] HTTP payload ") + payload);
    return _lastCode;
  }

  int sendRequest(const char* type, const String& payload) {
    _lastMethod = type ? type : "";
    _lastPayload = payload;
    _lastCode = 200;
    _lastResponse = "{}";
    Serial.println(String("[SIM] HTTP ") + _lastMethod + " " + _url);
    return _lastCode;
  }

  String getString() {
    return _lastResponse;
  }

  int getSize() const {
    return (int)_lastResponse.length();
  }

  bool connected() const {
    return _active;
  }

  void setTimeout(uint16_t timeout) {
    _timeout = timeout;
  }

  void useHTTP10(bool use = true) {
    _http10 = use;
  }

  String errorToString(int code) const {
    return String("HTTP error ") + String(code);
  }

  int writeToStream(FakeSerial* stream) {
    if (!stream) return 0;
    stream->print(_lastResponse);
    return (int)_lastResponse.length();
  }

private:
  bool _active = false;
  bool _http10 = false;
  uint16_t _timeout = 5000;
  int _lastCode = 0;
  String _url;
  String _lastMethod;
  String _lastPayload;
  String _lastResponse = "{}";
  std::map<String, String> _headers;
  String _authUser;
  String _authPassword;
};
