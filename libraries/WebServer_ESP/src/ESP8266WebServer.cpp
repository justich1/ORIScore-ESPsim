#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  // Windows headers must be included BEFORE Arduino.h.
  // Arduino defines INPUT/OUTPUT macros which would otherwise corrupt winuser.h.
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

#include "ESP8266WebServer.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <vector>

static String makeRouteKey(const String& uri, HTTPMethod method) {
  return String((int)method) + ":" + uri;
}

static String httpReason(int code) {
  switch (code) {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 303: return "See Other";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 500: return "Internal Server Error";
    default: return "OK";
  }
}

static int hexVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return -1;
}

static String urlDecode(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '+') {
      out.push_back(' ');
    }
    else if (s[i] == '%' && i + 2 < s.size()) {
      int a = hexVal(s[i + 1]);
      int b = hexVal(s[i + 2]);
      if (a >= 0 && b >= 0) {
        out.push_back((char)((a << 4) | b));
        i += 2;
      }
      else {
        out.push_back(s[i]);
      }
    }
    else {
      out.push_back(s[i]);
    }
  }
  return String(out);
}

static void parseArgsInto(const std::string& qs, std::map<String, String>& args) {
  size_t pos = 0;
  while (pos <= qs.size()) {
    size_t amp = qs.find('&', pos);
    std::string pair = qs.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
    if (!pair.empty()) {
      size_t eq = pair.find('=');
      std::string k = eq == std::string::npos ? pair : pair.substr(0, eq);
      std::string v = eq == std::string::npos ? "" : pair.substr(eq + 1);
      args[urlDecode(k)] = urlDecode(v);
    }
    if (amp == std::string::npos) break;
    pos = amp + 1;
  }
}

static HTTPMethod methodFromString(const std::string& m) {
  if (m == "GET") return HTTP_GET;
  if (m == "POST") return HTTP_POST;
  if (m == "PUT") return HTTP_PUT;
  if (m == "DELETE") return HTTP_DELETE;
  if (m == "PATCH") return HTTP_PATCH;
  return HTTP_ANY;
}

ESP8266WebServer::ESP8266WebServer(int p) : port(p) {}

ESP8266WebServer::~ESP8266WebServer() {
  stop();
}

void ESP8266WebServer::on(const String& uri, Handler handler) {
  handlers[makeRouteKey(uri, HTTP_ANY)] = handler;
}

void ESP8266WebServer::on(const String& uri, int method, Handler handler) {
  handlers[makeRouteKey(uri, (HTTPMethod)method)] = handler;
}

void ESP8266WebServer::on(const String& uri, HTTPMethod method, Handler handler) {
  handlers[makeRouteKey(uri, method)] = handler;
}

void ESP8266WebServer::on(const String& uri, HTTPMethod method, Handler handler, Handler uploadHandler) {
  handlers[makeRouteKey(uri, method)] = handler;
  uploadHandlers[makeRouteKey(uri, method)] = uploadHandler;
}

void ESP8266WebServer::onNotFound(Handler handler) {
  notFoundHandler = handler;
}

void ESP8266WebServer::begin() {
  if (httpRunning) return;

  const char* envPort = std::getenv("ORISIM_HTTP_PORT");
  listenPort = envPort && *envPort ? std::atoi(envPort) : 18088;
  if (listenPort <= 0) listenPort = 18088;

  httpRunning = true;
  httpThread = std::thread([this]() { httpLoop(); });
  httpThread.detach();

  std::cout << "HTTP listening on http://localhost:" << listenPort << "/" << std::endl;
}

void ESP8266WebServer::stop() {
  httpRunning = false;
}

void ESP8266WebServer::close() {
  stop();
}

void ESP8266WebServer::handleClient() {
  // Arduino WebServer spouští handler v loop() vlákně.
  // Starší ESPsim spouštěl handler přímo v HTTP vlákně, což u větších testů
  // čidel/webu vedlo ke kolizím s loop() a občas k 0xC0000005.
  // Proto HTTP vlákno jen zařadí požadavek a tady ho bezpečně obslouží firmware.
  while (true) {
    std::shared_ptr<PendingHttpRequest> req;

    {
      std::lock_guard<std::mutex> qlock(queueMutex);
      if (pendingRequests.empty()) break;
      req = pendingRequests.front();
      pendingRequests.pop();
    }

    if (!req) continue;

    {
      std::lock_guard<std::recursive_mutex> lock(stateMutex);
      currentUri = req->uri;
      currentMethod = req->method;
      currentArgs = req->args;
      response = "";
      responseContentType = "text/plain; charset=utf-8";
      statusCode = 0;
      headers.clear();
    }

    if (req->uri == "/favicon.ico") {
      send(204, "text/plain", "");
    }
    else {
      dispatchCurrentRequest();
    }

    {
      std::lock_guard<std::recursive_mutex> lock(stateMutex);
      req->code = statusCode == 0 ? 200 : statusCode;
      req->contentType = responseContentType.length() ? responseContentType : String("text/plain; charset=utf-8");
      req->body = response;
      req->responseHeaders = headers;
    }

    {
      std::lock_guard<std::mutex> doneLock(req->mutex);
      req->done = true;
    }
    req->cv.notify_one();
  }
}

void ESP8266WebServer::send(int code, const String& contentType, const String& content) {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  statusCode = code;
  responseContentType = contentType;
  response = content;
}

void ESP8266WebServer::send(int code, const char* contentType, const String& content) {
  send(code, String(contentType ? contentType : "text/plain; charset=utf-8"), content);
}

void ESP8266WebServer::send_P(int code, const char* contentType, const char* content) {
  send(code, contentType, String(content ? content : ""));
}

void ESP8266WebServer::sendHeader(const String& name, const String& value, bool first) {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  if (first) headers.clear();
  headers[name] = value;
}

void ESP8266WebServer::setContentLength(size_t len) {
  contentLength = len;
}

void ESP8266WebServer::sendContent(const String& content) {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  if (statusCode == 0) statusCode = 200;
  response += content;
}

bool ESP8266WebServer::hasArg(const String& name) const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return currentArgs.find(name) != currentArgs.end();
}

String ESP8266WebServer::arg(const String& name) const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  auto it = currentArgs.find(name);
  return it == currentArgs.end() ? String("") : it->second;
}

int ESP8266WebServer::args() const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return (int)currentArgs.size();
}

String ESP8266WebServer::argName(int i) const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  if (i < 0 || i >= (int)currentArgs.size()) return "";
  auto it = currentArgs.begin();
  std::advance(it, i);
  return it->first;
}

String ESP8266WebServer::uri() const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return currentUri;
}

HTTPMethod ESP8266WebServer::method() const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return currentMethod;
}

HTTPUpload& ESP8266WebServer::upload() {
  return currentUpload;
}

void ESP8266WebServer::simulateRequest(const String& u, int m, const std::map<String,String>& args) {
  {
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    currentUri = u;
    currentMethod = (HTTPMethod)m;
    currentArgs = args;
    response = "";
    responseContentType = "text/plain; charset=utf-8";
    statusCode = 0;
    headers.clear();
  }

  if (u == "/favicon.ico") {
    send(204, "text/plain", "");
  }
  else {
    dispatchCurrentRequest();
  }
}

String ESP8266WebServer::lastResponse() const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return response;
}

int ESP8266WebServer::lastStatusCode() const {
  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  return statusCode;
}

void ESP8266WebServer::dispatchCurrentRequest() {
  Handler handler;
  String key;
  String anyKey;

  {
    std::lock_guard<std::recursive_mutex> lock(stateMutex);
    key = makeRouteKey(currentUri, currentMethod);
    anyKey = makeRouteKey(currentUri, HTTP_ANY);

    auto it = handlers.find(key);
    if (it != handlers.end()) handler = it->second;
    else {
      it = handlers.find(anyKey);
      if (it != handlers.end()) handler = it->second;
      else if (notFoundHandler) handler = notFoundHandler;
    }
  }

  if (handler) {
    try {
      handler();
    }
    catch (const std::exception& ex) {
      send(500, "text/plain; charset=utf-8", String("Handler exception: ") + ex.what());
    }
    catch (...) {
      send(500, "text/plain; charset=utf-8", "Handler exception");
    }
  }
  else {
    send(404, "text/plain; charset=utf-8", String("Not found: ") + currentUri);
  }

  std::lock_guard<std::recursive_mutex> lock(stateMutex);
  if (statusCode == 0) statusCode = 200;
}

#ifdef _WIN32
void ESP8266WebServer::httpLoop() {
  WSADATA wsa{};
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    std::cout << "HTTP ERROR: WSAStartup failed" << std::endl;
    httpRunning = false;
    return;
  }

  SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (serverSock == INVALID_SOCKET) {
    std::cout << "HTTP ERROR: socket failed" << std::endl;
    WSACleanup();
    httpRunning = false;
    return;
  }

  BOOL reuse = TRUE;
  setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons((u_short)listenPort);

  if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    std::cout << "HTTP ERROR: bind failed on port " << listenPort << " (is fake runner still running?)" << std::endl;
    closesocket(serverSock);
    WSACleanup();
    httpRunning = false;
    return;
  }

  if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
    std::cout << "HTTP ERROR: listen failed" << std::endl;
    closesocket(serverSock);
    WSACleanup();
    httpRunning = false;
    return;
  }

  while (httpRunning) {
    SOCKET client = accept(serverSock, nullptr, nullptr);
    if (client == INVALID_SOCKET) continue;
    std::thread([this, client]() { handleRawHttpClient((void*)client); }).detach();
  }

  closesocket(serverSock);
  WSACleanup();
}

void ESP8266WebServer::handleRawHttpClient(void* socketHandle) {
  SOCKET client = (SOCKET)socketHandle;

  std::string req;
  char buf[4096];

  while (req.find("\r\n\r\n") == std::string::npos) {
    int n = recv(client, buf, sizeof(buf), 0);
    if (n <= 0) break;
    req.append(buf, buf + n);
    if (req.size() > 1024 * 1024) break;
  }

  size_t headerEnd = req.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    closesocket(client);
    return;
  }

  std::string headersText = req.substr(0, headerEnd);
  std::string body = req.substr(headerEnd + 4);

  std::istringstream hs(headersText);
  std::string requestLine;
  std::getline(hs, requestLine);
  if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();

  std::string methodText, target, version;
  {
    std::istringstream rl(requestLine);
    rl >> methodText >> target >> version;
  }

  int contentLen = 0;
  std::string headerLine;
  while (std::getline(hs, headerLine)) {
    if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
    std::string low = headerLine;
    std::transform(low.begin(), low.end(), low.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    const std::string prefix = "content-length:";
    if (low.rfind(prefix, 0) == 0) {
      contentLen = std::atoi(headerLine.substr(prefix.size()).c_str());
    }
  }

  while ((int)body.size() < contentLen) {
    int n = recv(client, buf, sizeof(buf), 0);
    if (n <= 0) break;
    body.append(buf, buf + n);
  }

  std::string path = target;
  std::string query;
  size_t q = target.find('?');
  if (q != std::string::npos) {
    path = target.substr(0, q);
    query = target.substr(q + 1);
  }

  std::map<String, String> parsedArgs;
  parseArgsInto(query, parsedArgs);

  HTTPMethod m = methodFromString(methodText);
  if (m == HTTP_POST || m == HTTP_PUT || m == HTTP_PATCH) {
    parsedArgs[String("plain")] = String(body);
    parseArgsInto(body, parsedArgs);
  }

  auto pending = std::make_shared<PendingHttpRequest>();
  pending->uri = urlDecode(path);
  pending->method = m;
  pending->args = parsedArgs;

  {
    std::lock_guard<std::mutex> qlock(queueMutex);
    pendingRequests.push(pending);
  }

  bool completed = false;
  {
    std::unique_lock<std::mutex> doneLock(pending->mutex);
    completed = pending->cv.wait_for(
      doneLock,
      std::chrono::seconds(10),
      [&]() { return pending->done; }
    );
  }

  int code;
  String reason;
  String contentType;
  String bodyOut;
  std::map<String, String> responseHeaders;

  if (!completed) {
    code = 503;
    contentType = "text/plain; charset=utf-8";
    bodyOut = "ESPsim HTTP request timeout: firmware loop() nevola server.handleClient() nebo je handler moc dlouhy.";
  }
  else {
    code = pending->code == 0 ? 200 : pending->code;
    contentType = pending->contentType.length() ? pending->contentType : String("text/plain; charset=utf-8");
    bodyOut = pending->body;
    responseHeaders = pending->responseHeaders;
  }

  reason = httpReason(code);

  std::ostringstream out;
  out << "HTTP/1.1 " << code << " " << reason.v << "\r\n";
  out << "Content-Type: " << contentType.v << "\r\n";
  out << "Content-Length: " << bodyOut.v.size() << "\r\n";
  out << "Connection: close\r\n";
  for (const auto& kv : responseHeaders) {
    out << kv.first.v << ": " << kv.second.v << "\r\n";
  }
  out << "\r\n";
  out << bodyOut.v;

  std::string data = out.str();
  ::send(client, data.c_str(), (int)data.size(), 0);
  closesocket(client);
}
#else
void ESP8266WebServer::httpLoop() {
  std::cout << "HTTP fake server is only implemented for Windows/MSVC in this build" << std::endl;
  httpRunning = false;
}

void ESP8266WebServer::handleRawHttpClient(void*) {}
#endif
