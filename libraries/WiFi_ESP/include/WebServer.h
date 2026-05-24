#pragma once

#include "Arduino.h"
#include "WiFi.h"
#include <map>
#include <functional>

#ifndef HTTP_GET
#define HTTP_GET 0
#endif

#ifndef HTTP_POST
#define HTTP_POST 1
#endif

#ifndef HTTP_ANY
#define HTTP_ANY -1
#endif

class WebServer {
public:
  using THandlerFunction = std::function<void(void)>;

  explicit WebServer(uint16_t port = 80)
    : _port(port) {}

  void begin() {
    _running = true;
    Serial.println(String("[SIM] WebServer.begin port=") + String((int)_port));
  }

  void close() {
    _running = false;
    Serial.println("[SIM] WebServer.close");
  }

  void stop() {
    close();
  }

  void handleClient() {
    // Simulátor zatím nepřijímá reálné HTTP požadavky.
    // Handler webu je uložený, aby se firmware chovalo normálně.
  }

  void on(const char* uri, THandlerFunction handler) {
    _routes[String(uri ? uri : "/")] = handler;
    Serial.println(String("[SIM] WebServer.on ") + String(uri ? uri : "/"));
  }

  void on(const String& uri, THandlerFunction handler) {
    on(uri.c_str(), handler);
  }

  void on(const char* uri, int method, THandlerFunction handler) {
    (void)method;
    on(uri, handler);
  }

  void on(const String& uri, int method, THandlerFunction handler) {
    (void)method;
    on(uri.c_str(), handler);
  }

  void onNotFound(THandlerFunction handler) {
    _notFound = handler;
  }

  void send(int code, const char* contentType = "text/plain", const String& content = String("")) {
    _lastCode = code;
    _lastContentType = contentType ? contentType : "";
    _lastContent = content;

    Serial.println(
      String("[SIM] WebServer.send code=") +
      String(code) +
      " type=" +
      _lastContentType +
      " bytes=" +
      String((int)_lastContent.length())
    );
  }

  void send(int code, const String& contentType, const String& content) {
    send(code, contentType.c_str(), content);
  }

  void send(int code, const char* contentType, const char* content) {
    send(code, contentType, String(content ? content : ""));
  }

  void sendHeader(const String& name, const String& value, bool first = false) {
    (void)first;
    _headers[name] = value;
    Serial.println(String("[SIM] WebServer.header ") + name + "=" + value);
  }

  void sendHeader(const char* name, const char* value, bool first = false) {
    sendHeader(String(name ? name : ""), String(value ? value : ""), first);
  }

  String arg(const String& name) const {
    auto it = _args.find(name);
    return it == _args.end() ? String("") : it->second;
  }

  String arg(const char* name) const {
    return arg(String(name ? name : ""));
  }

  String arg(int i) const {
    if (i < 0 || i >= (int)_args.size()) return String("");
    auto it = _args.begin();
    std::advance(it, i);
    return it->second;
  }

  String argName(int i) const {
    if (i < 0 || i >= (int)_args.size()) return String("");
    auto it = _args.begin();
    std::advance(it, i);
    return it->first;
  }

  int args() const {
    return (int)_args.size();
  }

  bool hasArg(const String& name) const {
    return _args.find(name) != _args.end();
  }

  bool hasArg(const char* name) const {
    return hasArg(String(name ? name : ""));
  }

  String uri() const {
    return _currentUri;
  }

  int method() const {
    return _currentMethod;
  }

  IPAddress client() const {
    return IPAddress();
  }

  // ESPsim helper pro pozdější testování handlerů bez browseru.
  // Příklad: server.simRequest("/save", {{"ssid","test"},{"pass","1234"}});
  void simSetArg(const String& name, const String& value) {
    _args[name] = value;
  }

  void simClearArgs() {
    _args.clear();
  }

  bool simRequest(const String& uri) {
    _currentUri = uri;
    auto it = _routes.find(uri);
    if (it != _routes.end() && it->second) {
      it->second();
      return true;
    }
    if (_notFound) {
      _notFound();
      return false;
    }
    return false;
  }

private:
  uint16_t _port = 80;
  bool _running = false;
  int _lastCode = 0;
  int _currentMethod = HTTP_GET;
  String _currentUri = "/";
  String _lastContentType;
  String _lastContent;
  std::map<String, String> _args;
  std::map<String, String> _headers;
  std::map<String, THandlerFunction> _routes;
  THandlerFunction _notFound = nullptr;
};
