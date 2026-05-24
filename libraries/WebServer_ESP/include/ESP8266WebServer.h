#pragma once
#include "Arduino.h"
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

#define CONTENT_LENGTH_UNKNOWN ((size_t)-1)

#ifndef HTTP_CODE_OK
#define HTTP_CODE_OK 200
#endif

#ifndef HTTP_CODE_FOUND
#define HTTP_CODE_FOUND 302
#endif

enum HTTPMethod {
  HTTP_ANY = 255,
  HTTP_GET = 0,
  HTTP_POST = 1,
  HTTP_PUT = 2,
  HTTP_DELETE = 3,
  HTTP_PATCH = 4
};

enum HTTPUploadStatus {
  UPLOAD_FILE_START,
  UPLOAD_FILE_WRITE,
  UPLOAD_FILE_END,
  UPLOAD_FILE_ABORTED
};

struct HTTPUpload {
  HTTPUploadStatus status = UPLOAD_FILE_START;
  String filename;
  String name;
  String type;
  size_t totalSize = 0;
  size_t currentSize = 0;
  uint8_t buf[2048]{};
};

class ESP8266WebServer {
public:
  using Handler = std::function<void()>;
  using THandlerFunction = std::function<void(void)>;

  explicit ESP8266WebServer(int port = 80);
  ~ESP8266WebServer();

  void on(const String& uri, Handler handler);
  void on(const char* uri, Handler handler) { on(String(uri), handler); }
  void on(const String& uri, int method, Handler handler);
  void on(const char* uri, int method, Handler handler) { on(String(uri), method, handler); }
  void on(const String& uri, HTTPMethod method, Handler handler);
  void on(const char* uri, HTTPMethod method, Handler handler) { on(String(uri), method, handler); }
  void on(const String& uri, HTTPMethod method, Handler handler, Handler uploadHandler);
  void on(const char* uri, HTTPMethod method, Handler handler, Handler uploadHandler) { on(String(uri), method, handler, uploadHandler); }

  void onNotFound(Handler handler);

  void begin();
  void stop();
  void close();
  void handleClient();

  void send(int code) { send(code, "text/plain", ""); }
  void send(int code, const String& contentType, const String& content);
  void send(int code, const char* contentType, const String& content);
  void send(int code, const char* contentType, const char* content) { send(code, contentType, String(content)); }
  void send_P(int code, const char* contentType, const char* content);
  void sendHeader(const String& name, const String& value, bool first = false);
  void sendHeader(const char* name, const char* value, bool first = false) { sendHeader(String(name), String(value), first); }
  void setContentLength(size_t len);
  void sendContent(const String& content);
  void sendContent(const char* content) { sendContent(String(content)); }

  bool hasArg(const String& name) const;
  bool hasArg(const char* name) const { return hasArg(String(name)); }
  String arg(const String& name) const;
  String arg(const char* name) const { return arg(String(name)); }
  int args() const;
  String argName(int i) const;

  String uri() const;
  HTTPMethod method() const;
  HTTPUpload& upload();

  void simulateRequest(const String& uri, int method = HTTP_GET, const std::map<String,String>& args = {});
  String lastResponse() const;
  int lastStatusCode() const;

private:
  int port;
  int listenPort = 0;
  std::map<String, Handler> handlers;
  std::map<String, Handler> uploadHandlers;
  Handler notFoundHandler;

  mutable std::recursive_mutex stateMutex;
  std::map<String, String> currentArgs;
  std::map<String, String> headers;
  String currentUri;
  HTTPMethod currentMethod = HTTP_GET;
  HTTPUpload currentUpload;
  String response;
  String responseContentType = "text/plain; charset=utf-8";
  int statusCode = 0;
  size_t contentLength = 0;

  std::atomic<bool> httpRunning{false};
  std::thread httpThread;

  void httpLoop();
  void handleRawHttpClient(void* socketHandle);
  void dispatchCurrentRequest();
};
