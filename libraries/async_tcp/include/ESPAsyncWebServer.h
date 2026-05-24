#pragma once

#include "Arduino.h"
#include "ESP8266WebServer.h"
#include <functional>
#include <memory>
#include <vector>

#ifndef ASYNCWEBSERVER_REGEX
#define ASYNCWEBSERVER_REGEX 0
#endif

class AsyncWebServerRequest;
class AsyncWebServerResponse;
class AsyncWebParameter;

using ArRequestHandlerFunction = std::function<void(AsyncWebServerRequest*)>;
using ArUploadHandlerFunction = std::function<void(AsyncWebServerRequest*, const String&, size_t, uint8_t*, size_t, bool)>;

class AsyncWebParameter {
public:
  AsyncWebParameter(const String& name = "", const String& value = "")
    : _name(name), _value(value) {}

  const String& name() const { return _name; }
  const String& value() const { return _value; }

private:
  String _name;
  String _value;
};

class AsyncWebServerResponse {
public:
  AsyncWebServerResponse(int code = 200, const String& contentType = "text/plain", const String& content = "")
    : _code(code), _contentType(contentType), _content(content) {}

  void addHeader(const String& name, const String& value) {
    _headers[name] = value;
  }

  void addHeader(const char* name, const char* value) {
    addHeader(String(name ? name : ""), String(value ? value : ""));
  }

  int code() const { return _code; }
  const String& contentType() const { return _contentType; }
  const String& content() const { return _content; }
  const std::map<String, String>& headers() const { return _headers; }

private:
  int _code = 200;
  String _contentType = "text/plain";
  String _content;
  std::map<String, String> _headers;
};

class AsyncWebServerRequest {
public:
  explicit AsyncWebServerRequest(ESP8266WebServer* server)
    : _server(server) {}

  bool hasParam(const String& name, bool post = false, bool file = false) const {
    (void)post;
    (void)file;
    return _server && _server->hasArg(name);
  }

  bool hasParam(const char* name, bool post = false, bool file = false) const {
    return hasParam(String(name ? name : ""), post, file);
  }

  AsyncWebParameter* getParam(const String& name, bool post = false, bool file = false) {
    (void)post;
    (void)file;
    String value = _server ? _server->arg(name) : String("");
    _params.emplace_back(std::make_unique<AsyncWebParameter>(name, value));
    return _params.back().get();
  }

  AsyncWebParameter* getParam(const char* name, bool post = false, bool file = false) {
    return getParam(String(name ? name : ""), post, file);
  }

  bool hasHeader(const String& name) const {
    // Cookie is assumed present after login in the simulator, so admin pages remain usable.
    if (name == "Cookie" || name == "cookie") return true;
    return false;
  }

  bool hasHeader(const char* name) const {
    return hasHeader(String(name ? name : ""));
  }

  String header(const String& name) const {
    if (name == "Cookie" || name == "cookie") return "logged_in=true";
    return "";
  }

  String header(const char* name) const {
    return header(String(name ? name : ""));
  }

  String url() const {
    return _server ? _server->uri() : String("");
  }

  HTTPMethod method() const {
    return _server ? _server->method() : HTTP_GET;
  }

  void send(int code, const String& contentType, const String& content) {
    if (_server) _server->send(code, contentType, content);
  }

  void send(int code, const char* contentType, const String& content) {
    send(code, String(contentType ? contentType : "text/plain"), content);
  }

  void send(int code, const char* contentType, const char* content) {
    send(code, String(contentType ? contentType : "text/plain"), String(content ? content : ""));
  }

  void send(AsyncWebServerResponse* response) {
    if (!response) return;

    if (_server) {
      for (const auto& kv : response->headers()) {
        _server->sendHeader(kv.first, kv.second, true);
      }

      _server->send(response->code(), response->contentType(), response->content());
    }

    delete response;
  }

  void redirect(const String& url) {
    if (!_server) return;
    _server->sendHeader("Location", url, true);
    _server->send(302, "text/plain", "");
  }

  void redirect(const char* url) {
    redirect(String(url ? url : "/"));
  }

  AsyncWebServerResponse* beginResponse(int code, const String& contentType, const String& content) {
    return new AsyncWebServerResponse(code, contentType, content);
  }

  AsyncWebServerResponse* beginResponse(int code, const char* contentType, const String& content) {
    return beginResponse(code, String(contentType ? contentType : "text/plain"), content);
  }

  AsyncWebServerResponse* beginResponse(int code, const char* contentType, const char* content) {
    return beginResponse(code, String(contentType ? contentType : "text/plain"), String(content ? content : ""));
  }

  AsyncWebServerResponse* beginResponse_P(int code, const char* contentType, const char* content) {
    return beginResponse(code, contentType, content);
  }

private:
  ESP8266WebServer* _server = nullptr;
  std::vector<std::unique_ptr<AsyncWebParameter>> _params;
};

class AsyncWebServer {
public:
  explicit AsyncWebServer(uint16_t port)
    : _server(port) {}

  void on(const String& uri, HTTPMethod method, ArRequestHandlerFunction handler) {
    _server.on(uri, method, [this, handler]() {
      AsyncWebServerRequest request(&_server);
      if (handler) handler(&request);
    });
  }

  void on(const char* uri, HTTPMethod method, ArRequestHandlerFunction handler) {
    on(String(uri ? uri : ""), method, handler);
  }

  void on(const String& uri, ArRequestHandlerFunction handler) {
    on(uri, HTTP_ANY, handler);
  }

  void on(const char* uri, ArRequestHandlerFunction handler) {
    on(String(uri ? uri : ""), HTTP_ANY, handler);
  }

  void onFileUpload(ArUploadHandlerFunction handler) {
    _uploadHandler = handler;
  }

  void begin() {
    _server.begin();
  }

  void end() {
    _server.stop();
  }

  void reset() {
    _server.stop();
  }

  void handleClient() {
    _server.handleClient();
  }

private:
  ESP8266WebServer _server;
  ArUploadHandlerFunction _uploadHandler;
};
