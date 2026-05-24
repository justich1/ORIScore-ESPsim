#include "LittleFS.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>

static std::map<String, String> gFiles;
static bool gLoaded = false;
FakeLittleFSClass LittleFS;

static std::filesystem::path fsRoot() {
  const char* env = std::getenv("ORISIM_FS_ROOT");
  if (env && *env) return std::filesystem::path(env);
  return std::filesystem::temp_directory_path() / "ORIScore_ESPsim_LittleFS";
}

static String normalizeFsPath(const String& p) {
  String s = p;
  s.replace("\\", "/");
  if (!s.startsWith("/")) s = String("/") + s;
  return s;
}

static std::filesystem::path hostPath(const String& p) {
  String norm = normalizeFsPath(p);
  std::string rel = norm.v;
  while (!rel.empty() && rel[0] == '/') rel.erase(rel.begin());
  return fsRoot() / rel;
}

static void saveOne(const String& p) {
  std::filesystem::path hp = hostPath(p);
  std::filesystem::create_directories(hp.parent_path());
  std::ofstream f(hp, std::ios::binary | std::ios::trunc);
  if (!f) return;
  auto it = gFiles.find(normalizeFsPath(p));
  if (it != gFiles.end()) f << it->second.v;
}

static void loadAll() {
  if (gLoaded) return;
  gLoaded = true;

  auto root = fsRoot();
  std::filesystem::create_directories(root);

  for (auto& entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;

    auto rel = std::filesystem::relative(entry.path(), root).generic_string();
    String key = String("/") + String(rel);

    std::ifstream f(entry.path(), std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    gFiles[key] = String(ss.str());
  }
}

File::File() {}

File::File(const String& p, const String& m) : path(normalizeFsPath(p)), mode(m), opened(false) {
  loadAll();

  bool writeMode = m.indexOf("w") >= 0;
  bool appendMode = m.indexOf("a") >= 0;
  bool readMode = !writeMode && !appendMode;

  if (writeMode) {
    gFiles[path] = "";
    opened = true;
  }
  else if (appendMode) {
    if (gFiles.find(path) == gFiles.end()) gFiles[path] = "";
    opened = true;
  }
  else if (readMode) {
    opened = gFiles.find(path) != gFiles.end();
  }
}

File::operator bool() const { return opened; }

size_t File::write(uint8_t b) {
  if (!opened) return 0;
  gFiles[path] += String((char)b);
  return 1;
}

size_t File::write(const uint8_t* data, size_t len) {
  if (!opened) return 0;
  for (size_t i = 0; i < len; i++) write(data[i]);
  return len;
}

size_t File::print(const String& s) {
  if (!opened) return 0;
  gFiles[path] += s;
  return s.length();
}

size_t File::print(const char* s) { return print(String(s)); }

String File::readString() {
  if (!opened) return "";
  auto it = gFiles.find(path);
  return it == gFiles.end() ? String("") : it->second;
}

int File::available() {
  if (!opened) return 0;
  auto it = gFiles.find(path);
  if (it == gFiles.end()) return 0;
  return readPos < it->second.length() ? (int)(it->second.length() - readPos) : 0;
}

int File::read() {
  if (!opened) return -1;
  auto it = gFiles.find(path);
  if (it == gFiles.end()) return -1;
  if (readPos >= it->second.length()) return -1;
  return (unsigned char)it->second[readPos++];
}


size_t File::size() {
  if (!opened) return 0;

  auto it = gFiles.find(path);
  if (it == gFiles.end()) return 0;

  return it->second.length();
}

size_t File::readBytes(char* buffer, size_t length) {
  if (!buffer || !opened) return 0;

  size_t count = 0;

  while (count < length && available() > 0) {
    int c = read();
    if (c < 0) break;
    buffer[count++] = (char)c;
  }

  return count;
}

size_t File::readBytes(uint8_t* buffer, size_t length) {
  return readBytes((char*)buffer, length);
}

void File::close() {
  if (opened && (mode.indexOf("w") >= 0 || mode.indexOf("a") >= 0)) {
    saveOne(path);
  }
  opened = false;
}

String File::name() const { return path; }

bool FakeLittleFSClass::begin(bool formatOnFail) {
  loadAll();
  std::filesystem::create_directories(fsRoot());
  std::cout << "LittleFS root " << fsRoot().string() << std::endl;
  return true;
}

bool FakeLittleFSClass::exists(const String& path) {
  loadAll();
  return gFiles.find(normalizeFsPath(path)) != gFiles.end();
}

File FakeLittleFSClass::open(const String& path, const String& mode) { return File(path, mode); }

void FakeLittleFSClass::remove(const String& path) {
  loadAll();
  String key = normalizeFsPath(path);
  gFiles.erase(key);
  std::error_code ec;
  std::filesystem::remove(hostPath(key), ec);
}

void FakeLittleFSClass::setFile(const String& path, const String& content) {
  loadAll();
  String key = normalizeFsPath(path);
  gFiles[key] = content;
  saveOne(key);
}

String FakeLittleFSClass::getFile(const String& path) {
  loadAll();
  String key = normalizeFsPath(path);
  auto it = gFiles.find(key);
  return it == gFiles.end() ? String("") : it->second;
}
