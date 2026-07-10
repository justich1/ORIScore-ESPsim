#pragma once

#include "Arduino.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <map>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

class JsonVariant;
class JsonObject;
class JsonArray;
class JsonDocumentBase;

class DeserializationError {
public:
  DeserializationError(bool failed = false) : _failed(failed) {}
  operator bool() const { return _failed; }
  const char* c_str() const { return _failed ? "Fake deserialization error" : "Ok"; }
  const char* f_str() const { return c_str(); }
private:
  bool _failed;
};

enum class FakeJsonType { Null, Bool, Int, Double, String, Object, Array };

struct FakeJsonNode {
  FakeJsonType type = FakeJsonType::Null;
  String s;
  bool b = false;
  long long i = 0;
  double d = 0.0;
  std::map<String, std::shared_ptr<FakeJsonNode>> object;
  std::vector<std::shared_ptr<FakeJsonNode>> array;
};

inline std::shared_ptr<FakeJsonNode> fakeJsonNode() {
  return std::make_shared<FakeJsonNode>();
}

inline void fakeJsonSetNull(const std::shared_ptr<FakeJsonNode>& n) {
  if (!n) return;
  n->type = FakeJsonType::Null;
  n->s = "";
  n->b = false;
  n->i = 0;
  n->d = 0;
  n->object.clear();
  n->array.clear();
}

class JsonVariant {
public:
  JsonVariant() : _node(fakeJsonNode()) {}
  explicit JsonVariant(std::shared_ptr<FakeJsonNode> node) : _node(node ? node : fakeJsonNode()) {}
  JsonVariant(std::nullptr_t) : _node(fakeJsonNode()) { setNull(); }
  JsonVariant(const char* s) : _node(fakeJsonNode()) { *this = s; }
  JsonVariant(const String& s) : _node(fakeJsonNode()) { *this = s; }
  JsonVariant(bool v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(char v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(signed char v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(unsigned char v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(short v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(unsigned short v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(int v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(unsigned int v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(long v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(unsigned long v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(long long v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(unsigned long long v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(float v) : _node(fakeJsonNode()) { *this = v; }
  JsonVariant(double v) : _node(fakeJsonNode()) { *this = v; }

  JsonVariant& operator=(std::nullptr_t) { setNull(); return *this; }
  JsonVariant& operator=(const char* v) { _node->type = FakeJsonType::String; _node->s = v ? v : ""; return *this; }
  JsonVariant& operator=(const String& v) { _node->type = FakeJsonType::String; _node->s = v; return *this; }
  JsonVariant& operator=(bool v) { _node->type = FakeJsonType::Bool; _node->b = v; _node->i = v ? 1 : 0; _node->d = v ? 1.0 : 0.0; return *this; }
  JsonVariant& operator=(char v) { return (*this = (long long)v); }
  JsonVariant& operator=(signed char v) { return (*this = (long long)v); }
  JsonVariant& operator=(unsigned char v) { return (*this = (unsigned long long)v); }
  JsonVariant& operator=(short v) { return (*this = (long long)v); }
  JsonVariant& operator=(unsigned short v) { return (*this = (unsigned long long)v); }
  JsonVariant& operator=(int v) { _node->type = FakeJsonType::Int; _node->i = v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(unsigned int v) { _node->type = FakeJsonType::Int; _node->i = (long long)v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(long v) { _node->type = FakeJsonType::Int; _node->i = v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(unsigned long v) { _node->type = FakeJsonType::Int; _node->i = (long long)v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(long long v) { _node->type = FakeJsonType::Int; _node->i = v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(unsigned long long v) { _node->type = FakeJsonType::Int; _node->i = (long long)v; _node->d = (double)v; return *this; }
  JsonVariant& operator=(float v) { _node->type = FakeJsonType::Double; _node->i = (long long)v; _node->d = v; return *this; }
  JsonVariant& operator=(double v) { _node->type = FakeJsonType::Double; _node->i = (long long)v; _node->d = v; return *this; }
  JsonVariant& operator=(const JsonObject& obj);
  JsonVariant& operator=(const JsonArray& arr);

  JsonVariant operator[](const char* key);
  JsonVariant operator[](const String& key);
  JsonVariant operator[](int idx);
  JsonVariant operator[](size_t idx);
  JsonVariant operator[](const char* key) const;
  JsonVariant operator[](const String& key) const;
  JsonVariant operator[](int idx) const;
  JsonVariant operator[](size_t idx) const;

  bool containsKey(const char* key) const;
  bool containsKey(const String& key) const;

  JsonObject createNestedObject(const char* key);
  JsonObject createNestedObject(const String& key);
  JsonObject createNestedObject();

  JsonArray createNestedArray(const char* key);
  JsonArray createNestedArray(const String& key);
  JsonArray createNestedArray();

  JsonVariant add();

  template <typename T>
  JsonVariant add(const T& value);

  template <size_t N>
  const char* operator|(const char (&def)[N]) const { return _node->type == FakeJsonType::String ? _node->s.c_str() : def; }
  const char* operator|(const char* def) const { return _node->type == FakeJsonType::String ? _node->s.c_str() : (def ? def : ""); }
  String operator|(const String& def) const { return _node->type == FakeJsonType::String ? _node->s : def; }
  bool operator|(bool def) const { return _node->type != FakeJsonType::Null ? as<bool>() : def; }
  char operator|(char def) const { return _node->type != FakeJsonType::Null ? as<char>() : def; }
  signed char operator|(signed char def) const { return _node->type != FakeJsonType::Null ? as<signed char>() : def; }
  unsigned char operator|(unsigned char def) const { return _node->type != FakeJsonType::Null ? as<unsigned char>() : def; }
  short operator|(short def) const { return _node->type != FakeJsonType::Null ? as<short>() : def; }
  unsigned short operator|(unsigned short def) const { return _node->type != FakeJsonType::Null ? as<unsigned short>() : def; }
  int operator|(int def) const { return _node->type != FakeJsonType::Null ? as<int>() : def; }
  unsigned int operator|(unsigned int def) const { return _node->type != FakeJsonType::Null ? as<unsigned int>() : def; }
  long operator|(long def) const { return _node->type != FakeJsonType::Null ? as<long>() : def; }
  unsigned long operator|(unsigned long def) const { return _node->type != FakeJsonType::Null ? as<unsigned long>() : def; }
  long long operator|(long long def) const { return _node->type != FakeJsonType::Null ? as<long long>() : def; }
  unsigned long long operator|(unsigned long long def) const { return _node->type != FakeJsonType::Null ? as<unsigned long long>() : def; }
  float operator|(float def) const { return _node->type != FakeJsonType::Null ? as<float>() : def; }
  double operator|(double def) const { return _node->type != FakeJsonType::Null ? as<double>() : def; }

  template <typename T>
  T as() const {
    if constexpr (std::is_same_v<T, String>) return _node->s;
    else if constexpr (std::is_same_v<T, const char*>) return _node->s.c_str();
    else if constexpr (std::is_same_v<T, bool>) {
      if (_node->type == FakeJsonType::Bool) return _node->b;
      if (_node->type == FakeJsonType::String) return _node->s == "true" || _node->s == "1" || _node->s == "ON";
      return _node->i != 0 || _node->d != 0.0;
    }
    else if constexpr (std::is_integral_v<T>) {
      if (_node->type == FakeJsonType::String) return (T)_node->s.toInt();
      return (T)_node->i;
    }
    else if constexpr (std::is_floating_point_v<T>) {
      if (_node->type == FakeJsonType::String) return (T)_node->s.toFloat();
      return (T)_node->d;
    }
    else return T{};
  }

  bool isNull() const { return _node->type == FakeJsonType::Null; }
  FakeJsonType type() const { return _node->type; }
  const String& stringValue() const { return _node->s; }
  bool boolValue() const { return _node->b; }
  long long intValue() const { return _node->i; }
  double doubleValue() const { return _node->d; }
  std::shared_ptr<FakeJsonNode> node() const { return _node; }

  operator const char*() const { return _node->s.c_str(); }
  operator String() const { return _node->s; }
  operator bool() const { return as<bool>(); }
  operator char() const { return as<char>(); }
  operator signed char() const { return as<signed char>(); }
  operator unsigned char() const { return as<unsigned char>(); }
  operator short() const { return as<short>(); }
  operator unsigned short() const { return as<unsigned short>(); }
  operator int() const { return as<int>(); }
  operator unsigned int() const { return as<unsigned int>(); }
  operator long() const { return as<long>(); }
  operator unsigned long() const { return as<unsigned long>(); }
  operator long long() const { return as<long long>(); }
  operator unsigned long long() const { return as<unsigned long long>(); }
  operator float() const { return as<float>(); }
  operator double() const { return as<double>(); }

  operator JsonObject() const;
  operator JsonArray() const;

private:
  void setNull() { fakeJsonSetNull(_node); }
  std::shared_ptr<FakeJsonNode> _node;
};

class JsonObject {
public:
  JsonObject() : _node(fakeJsonNode()) { _node->type = FakeJsonType::Object; }
  explicit JsonObject(std::shared_ptr<FakeJsonNode> node) : _node(node ? node : fakeJsonNode()) { _node->type = FakeJsonType::Object; }

  JsonVariant operator[](const char* key) {
    ensureObject();
    String k(key ? key : "");
    auto& child = _node->object[k];
    if (!child) child = fakeJsonNode();
    return JsonVariant(child);
  }
  JsonVariant operator[](const String& key) {
    ensureObject();
    auto& child = _node->object[key];
    if (!child) child = fakeJsonNode();
    return JsonVariant(child);
  }
  JsonVariant operator[](const char* key) const {
    if (_node->type != FakeJsonType::Object) return JsonVariant();
    auto it = _node->object.find(String(key ? key : ""));
    return it == _node->object.end() ? JsonVariant() : JsonVariant(it->second);
  }
  JsonVariant operator[](const String& key) const {
    if (_node->type != FakeJsonType::Object) return JsonVariant();
    auto it = _node->object.find(key);
    return it == _node->object.end() ? JsonVariant() : JsonVariant(it->second);
  }

  bool containsKey(const char* key) const { return _node->type == FakeJsonType::Object && _node->object.find(String(key ? key : "")) != _node->object.end(); }
  bool containsKey(const String& key) const { return _node->type == FakeJsonType::Object && _node->object.find(key) != _node->object.end(); }

  JsonObject createNestedObject(const char* key) {
    ensureObject();
    String k(key ? key : "");
    auto child = fakeJsonNode();
    child->type = FakeJsonType::Object;
    _node->object[k] = child;
    return JsonObject(child);
  }
  JsonObject createNestedObject(const String& key) { return createNestedObject(key.c_str()); }

  JsonArray createNestedArray(const char* key);
  JsonArray createNestedArray(const String& key);

  const std::map<String, std::shared_ptr<FakeJsonNode>>& values() const { return _node->object; }
  std::shared_ptr<FakeJsonNode> node() const { return _node; }

private:
  void ensureObject() const { if (_node->type != FakeJsonType::Object) { _node->type = FakeJsonType::Object; _node->object.clear(); } }
  std::shared_ptr<FakeJsonNode> _node;
};

class JsonArray {
public:
  JsonArray() : _node(fakeJsonNode()) { _node->type = FakeJsonType::Array; }
  explicit JsonArray(std::shared_ptr<FakeJsonNode> node) : _node(node ? node : fakeJsonNode()) { _node->type = FakeJsonType::Array; }

  // Compatibility with ArduinoJson API used by generated firmware.
  // Real ArduinoJson JsonArray supports isNull() and range-based for loops.
  bool isNull() const { return !_node || _node->type == FakeJsonType::Null; }
  operator bool() const { return !isNull(); }

  class iterator {
  public:
    using Inner = std::vector<std::shared_ptr<FakeJsonNode>>::iterator;

    iterator() = default;
    explicit iterator(Inner it) : _it(it) {}

    JsonVariant operator*() const { return JsonVariant(*_it); }
    iterator& operator++() { ++_it; return *this; }
    iterator operator++(int) { iterator tmp(*this); ++_it; return tmp; }

    bool operator==(const iterator& other) const { return _it == other._it; }
    bool operator!=(const iterator& other) const { return _it != other._it; }

  private:
    Inner _it;
  };

  class const_iterator {
  public:
    using Inner = std::vector<std::shared_ptr<FakeJsonNode>>::const_iterator;

    const_iterator() = default;
    explicit const_iterator(Inner it) : _it(it) {}

    JsonVariant operator*() const { return JsonVariant(*_it); }
    const_iterator& operator++() { ++_it; return *this; }
    const_iterator operator++(int) { const_iterator tmp(*this); ++_it; return tmp; }

    bool operator==(const const_iterator& other) const { return _it == other._it; }
    bool operator!=(const const_iterator& other) const { return _it != other._it; }

  private:
    Inner _it;
  };

  iterator begin() { ensureArray(); return iterator(_node->array.begin()); }
  iterator end() { ensureArray(); return iterator(_node->array.end()); }
  const_iterator begin() const { ensureArray(); return const_iterator(_node->array.begin()); }
  const_iterator end() const { ensureArray(); return const_iterator(_node->array.end()); }
  const_iterator cbegin() const { ensureArray(); return const_iterator(_node->array.begin()); }
  const_iterator cend() const { ensureArray(); return const_iterator(_node->array.end()); }

  JsonObject createNestedObject() {
    ensureArray();
    auto child = fakeJsonNode();
    child->type = FakeJsonType::Object;
    _node->array.push_back(child);
    return JsonObject(child);
  }

  JsonArray createNestedArray() {
    ensureArray();
    auto child = fakeJsonNode();
    child->type = FakeJsonType::Array;
    _node->array.push_back(child);
    return JsonArray(child);
  }

  JsonVariant operator[](size_t idx) {
    ensureArray();
    while (idx >= _node->array.size()) _node->array.push_back(fakeJsonNode());
    return JsonVariant(_node->array[idx]);
  }
  JsonVariant operator[](size_t idx) const {
    if (_node->type != FakeJsonType::Array || idx >= _node->array.size()) return JsonVariant();
    return JsonVariant(_node->array[idx]);
  }

  void add(const String& v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(const char* v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(bool v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(char v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(signed char v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(unsigned char v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(short v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(unsigned short v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(int v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(unsigned int v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(long v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(unsigned long v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(long long v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(unsigned long long v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(float v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }
  void add(double v) { auto n = fakeJsonNode(); JsonVariant tmp(n); tmp = v; _node->array.push_back(n); }

  size_t size() const { return _node->type == FakeJsonType::Array ? _node->array.size() : 0; }
  const std::vector<std::shared_ptr<FakeJsonNode>>& items() const { return _node->array; }
  std::shared_ptr<FakeJsonNode> node() const { return _node; }

private:
  void ensureArray() const { if (_node->type != FakeJsonType::Array) { _node->type = FakeJsonType::Array; _node->array.clear(); } }
  std::shared_ptr<FakeJsonNode> _node;
};


inline JsonVariant JsonVariant::operator[](const char* key) {
  if (_node->type != FakeJsonType::Object) {
    _node->type = FakeJsonType::Object;
    _node->object.clear();
    _node->array.clear();
  }

  String k(key ? key : "");
  auto& child = _node->object[k];
  if (!child) child = fakeJsonNode();
  return JsonVariant(child);
}

inline JsonVariant JsonVariant::operator[](const String& key) {
  return (*this)[key.c_str()];
}

inline JsonVariant JsonVariant::operator[](int idx) {
  return (*this)[(size_t)(idx < 0 ? 0 : idx)];
}

inline JsonVariant JsonVariant::operator[](size_t idx) {
  if (_node->type != FakeJsonType::Array) {
    _node->type = FakeJsonType::Array;
    _node->array.clear();
    _node->object.clear();
  }

  while (idx >= _node->array.size()) {
    _node->array.push_back(fakeJsonNode());
  }

  return JsonVariant(_node->array[idx]);
}

inline JsonVariant JsonVariant::operator[](const char* key) const {
  if (_node->type != FakeJsonType::Object)
    return JsonVariant();

  auto it = _node->object.find(String(key ? key : ""));
  return it == _node->object.end() ? JsonVariant() : JsonVariant(it->second);
}

inline JsonVariant JsonVariant::operator[](const String& key) const {
  return (*this)[key.c_str()];
}

inline JsonVariant JsonVariant::operator[](int idx) const {
  return (*this)[(size_t)(idx < 0 ? 0 : idx)];
}

inline JsonVariant JsonVariant::operator[](size_t idx) const {
  if (_node->type != FakeJsonType::Array || idx >= _node->array.size())
    return JsonVariant();

  return JsonVariant(_node->array[idx]);
}

inline bool JsonVariant::containsKey(const char* key) const {
  if (_node->type != FakeJsonType::Object)
    return false;

  return _node->object.find(String(key ? key : "")) != _node->object.end();
}

inline bool JsonVariant::containsKey(const String& key) const {
  return containsKey(key.c_str());
}

inline JsonObject JsonVariant::createNestedObject(const char* key) {
  JsonVariant child = (*this)[key];
  auto n = child.node();
  n->type = FakeJsonType::Object;
  n->object.clear();
  n->array.clear();
  return JsonObject(n);
}

inline JsonObject JsonVariant::createNestedObject(const String& key) {
  return createNestedObject(key.c_str());
}

inline JsonObject JsonVariant::createNestedObject() {
  if (_node->type != FakeJsonType::Array) {
    _node->type = FakeJsonType::Array;
    _node->array.clear();
    _node->object.clear();
  }

  auto child = fakeJsonNode();
  child->type = FakeJsonType::Object;
  _node->array.push_back(child);
  return JsonObject(child);
}

inline JsonArray JsonVariant::createNestedArray(const char* key) {
  JsonVariant child = (*this)[key];
  auto n = child.node();
  n->type = FakeJsonType::Array;
  n->array.clear();
  n->object.clear();
  return JsonArray(n);
}

inline JsonArray JsonVariant::createNestedArray(const String& key) {
  return createNestedArray(key.c_str());
}

inline JsonArray JsonVariant::createNestedArray() {
  if (_node->type != FakeJsonType::Array) {
    _node->type = FakeJsonType::Array;
    _node->array.clear();
    _node->object.clear();
  }

  auto child = fakeJsonNode();
  child->type = FakeJsonType::Array;
  _node->array.push_back(child);
  return JsonArray(child);
}

inline JsonVariant JsonVariant::add() {
  if (_node->type != FakeJsonType::Array) {
    _node->type = FakeJsonType::Array;
    _node->array.clear();
    _node->object.clear();
  }

  auto child = fakeJsonNode();
  _node->array.push_back(child);
  return JsonVariant(child);
}

template <typename T>
inline JsonVariant JsonVariant::add(const T& value) {
  JsonVariant item = add();
  item = value;
  return item;
}

inline JsonVariant& JsonVariant::operator=(const JsonObject& obj) { _node = obj.node(); return *this; }
inline JsonVariant& JsonVariant::operator=(const JsonArray& arr) { _node = arr.node(); return *this; }
inline JsonVariant::operator JsonObject() const { return JsonObject(_node); }
inline JsonVariant::operator JsonArray() const { return JsonArray(_node); }

// Compatibility helpers for code using ArduinoJson v6/v7 style:
// JsonArray arr = variant.as<JsonArray>();
// JsonObject obj = variant.as<JsonObject>();
template <>
inline JsonObject JsonVariant::as<JsonObject>() const { return JsonObject(_node); }

template <>
inline JsonArray JsonVariant::as<JsonArray>() const { return JsonArray(_node); }

inline JsonArray JsonObject::createNestedArray(const char* key) {
  ensureObject();
  String k(key ? key : "");
  auto child = fakeJsonNode();
  child->type = FakeJsonType::Array;
  _node->object[k] = child;
  return JsonArray(child);
}
inline JsonArray JsonObject::createNestedArray(const String& key) { return createNestedArray(key.c_str()); }

class JsonDocumentBase : public JsonObject {
public:
  explicit JsonDocumentBase(size_t = 0) : JsonObject(fakeJsonNode()), _root(node()) { _root->type = FakeJsonType::Object; }

  template <typename T>
  T to() {
    if constexpr (std::is_same_v<T, JsonArray>) {
      _root->type = FakeJsonType::Array;
      _root->array.clear();
      return JsonArray(_root);
    }
    else if constexpr (std::is_same_v<T, JsonObject>) {
      _root->type = FakeJsonType::Object;
      _root->object.clear();
      return JsonObject(_root);
    }
    else return T();
  }

  template <typename T>
  T as() { return T(); }

  void clear() { fakeJsonSetNull(_root); _root->type = FakeJsonType::Object; }
  size_t memoryUsage() const { return 0; }
  bool isRootArray() const { return _root->type == FakeJsonType::Array; }
  JsonArray rootArray() const { return JsonArray(_root); }
  JsonObject rootObject() const { return JsonObject(_root); }
  std::shared_ptr<FakeJsonNode> rootNode() const { return _root; }

private:
  std::shared_ptr<FakeJsonNode> _root;
};

class DynamicJsonDocument : public JsonDocumentBase {
public:
  explicit DynamicJsonDocument(size_t size) : JsonDocumentBase(size) {}
};

template <size_t N>
class StaticJsonDocument : public JsonDocumentBase {
public:
  StaticJsonDocument() : JsonDocumentBase(N) {}
};

inline String fakeJsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (char c : in.v) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

inline String fakeJsonNumber(double d) {
  if (std::isnan(d)) return "null";
  std::ostringstream oss;
  oss << std::setprecision(10) << d;
  return String(oss.str());
}

inline String fakeJsonSerializeNode(const std::shared_ptr<FakeJsonNode>& n);

inline String fakeJsonSerializeObjectNode(const std::shared_ptr<FakeJsonNode>& n) {
  String out = "{";
  bool first = true;
  if (n && n->type == FakeJsonType::Object) {
    for (const auto& kv : n->object) {
      if (!first) out += ",";
      first = false;
      out += "\"";
      out += fakeJsonEscape(kv.first);
      out += "\":";
      out += fakeJsonSerializeNode(kv.second);
    }
  }
  out += "}";
  return out;
}

inline String fakeJsonSerializeArrayNode(const std::shared_ptr<FakeJsonNode>& n) {
  String out = "[";
  bool first = true;
  if (n && n->type == FakeJsonType::Array) {
    for (const auto& item : n->array) {
      if (!first) out += ",";
      first = false;
      out += fakeJsonSerializeNode(item);
    }
  }
  out += "]";
  return out;
}

inline String fakeJsonSerializeNode(const std::shared_ptr<FakeJsonNode>& n) {
  if (!n) return "null";
  switch (n->type) {
    case FakeJsonType::Null: return "null";
    case FakeJsonType::Bool: return n->b ? "true" : "false";
    case FakeJsonType::Int: return String((long long)n->i);
    case FakeJsonType::Double: return fakeJsonNumber(n->d);
    case FakeJsonType::String: return String("\"") + fakeJsonEscape(n->s) + "\"";
    case FakeJsonType::Object: return fakeJsonSerializeObjectNode(n);
    case FakeJsonType::Array: return fakeJsonSerializeArrayNode(n);
  }
  return "null";
}

class FakeJsonParser {
public:
  explicit FakeJsonParser(const char* text) : p(text ? text : "") {}

  std::shared_ptr<FakeJsonNode> parse() {
    skipWs();
    auto n = parseValue();
    skipWs();
    return n;
  }

private:
  const char* p;

  void skipWs() { while (*p && std::isspace((unsigned char)*p)) ++p; }
  bool consume(char c) { skipWs(); if (*p == c) { ++p; return true; } return false; }

  std::shared_ptr<FakeJsonNode> parseValue() {
    skipWs();
    if (*p == '{') return parseObject();
    if (*p == '[') return parseArray();
    if (*p == '"') return parseStringNode();
    if (*p == '-' || std::isdigit((unsigned char)*p)) return parseNumber();
    if (match("true")) { auto n=fakeJsonNode(); JsonVariant tmp(n); tmp=true; return n; }
    if (match("false")) { auto n=fakeJsonNode(); JsonVariant tmp(n); tmp=false; return n; }
    if (match("null")) return fakeJsonNode();
    return fakeJsonNode();
  }

  bool match(const char* s) {
    const char* q = p;
    while (*s && *q == *s) { ++q; ++s; }
    if (*s == 0) { p = q; return true; }
    return false;
  }

  std::shared_ptr<FakeJsonNode> parseObject() {
    auto n = fakeJsonNode();
    n->type = FakeJsonType::Object;
    consume('{');
    skipWs();
    if (consume('}')) return n;
    while (*p) {
      auto keyNode = parseStringNode();
      String key = keyNode->s;
      consume(':');
      n->object[key] = parseValue();
      skipWs();
      if (consume('}')) break;
      consume(',');
    }
    return n;
  }

  std::shared_ptr<FakeJsonNode> parseArray() {
    auto n = fakeJsonNode();
    n->type = FakeJsonType::Array;
    consume('[');
    skipWs();
    if (consume(']')) return n;
    while (*p) {
      n->array.push_back(parseValue());
      skipWs();
      if (consume(']')) break;
      consume(',');
    }
    return n;
  }

  std::shared_ptr<FakeJsonNode> parseStringNode() {
    auto n = fakeJsonNode();
    n->type = FakeJsonType::String;
    consume('"');
    String out;
    while (*p && *p != '"') {
      char c = *p++;
      if (c == '\\' && *p) {
        char e = *p++;
        switch (e) {
          case 'n': out += '\n'; break;
          case 'r': out += '\r'; break;
          case 't': out += '\t'; break;
          case '"': out += '"'; break;
          case '\\': out += '\\'; break;
          default: out += e; break;
        }
      } else {
        out += c;
      }
    }
    consume('"');
    n->s = out;
    return n;
  }

  std::shared_ptr<FakeJsonNode> parseNumber() {
    const char* start = p;
    if (*p == '-') ++p;
    while (std::isdigit((unsigned char)*p)) ++p;
    bool isDouble = false;
    if (*p == '.') { isDouble = true; ++p; while (std::isdigit((unsigned char)*p)) ++p; }
    if (*p == 'e' || *p == 'E') { isDouble = true; ++p; if (*p == '+' || *p == '-') ++p; while (std::isdigit((unsigned char)*p)) ++p; }
    std::string s(start, p - start);
    auto n = fakeJsonNode();
    if (isDouble) { JsonVariant tmp(n); tmp = std::atof(s.c_str()); }
    else { JsonVariant tmp(n); tmp = (long long)std::atoll(s.c_str()); }
    return n;
  }
};

inline DeserializationError fakeDeserialize(JsonDocumentBase& doc, const String& text) {
  FakeJsonParser parser(text.c_str());
  auto n = parser.parse();
  if (!n) return DeserializationError(true);
  *doc.rootNode() = *n;
  return DeserializationError(false);
}

inline DeserializationError deserializeJson(JsonDocumentBase& doc, const String& text) { return fakeDeserialize(doc, text); }
inline DeserializationError deserializeJson(JsonDocumentBase& doc, String& text) { return fakeDeserialize(doc, text); }
inline DeserializationError deserializeJson(JsonDocumentBase& doc, const char* text) { return fakeDeserialize(doc, String(text ? text : "")); }

template <size_t N>
DeserializationError deserializeJson(JsonDocumentBase& doc, const char (&text)[N]) {
  (void)N;
  return fakeDeserialize(doc, String(text));
}

template <size_t N>
DeserializationError deserializeJson(JsonDocumentBase& doc, char (&text)[N]) {
  (void)N;
  return fakeDeserialize(doc, String(text));
}

template <typename T>
DeserializationError deserializeJson(JsonDocumentBase& doc, T& input) {
  String s;
  while (input.available()) {
    int c = input.read();
    if (c < 0) break;
    s += (char)c;
  }
  return fakeDeserialize(doc, s);
}

inline size_t serializeJson(const JsonDocumentBase& doc, String& out) { out = fakeJsonSerializeNode(doc.rootNode()); return out.length(); }
inline size_t serializeJson(const JsonObject& obj, String& out) { out = fakeJsonSerializeNode(obj.node()); return out.length(); }
inline size_t serializeJson(const JsonArray& arr, String& out) { out = fakeJsonSerializeNode(arr.node()); return out.length(); }


template <size_t N>
size_t serializeJson(const JsonDocumentBase& doc, char (&out)[N]) {
  String s;
  size_t n = serializeJson(doc, s);
  if (N > 0) {
    std::strncpy(out, s.c_str(), N - 1);
    out[N - 1] = '\0';
  }
  return n;
}

template <size_t N>
size_t serializeJson(const JsonObject& obj, char (&out)[N]) {
  String s;
  size_t n = serializeJson(obj, s);
  if (N > 0) {
    std::strncpy(out, s.c_str(), N - 1);
    out[N - 1] = '\0';
  }
  return n;
}

template <size_t N>
size_t serializeJson(const JsonArray& arr, char (&out)[N]) {
  String s;
  size_t n = serializeJson(arr, s);
  if (N > 0) {
    std::strncpy(out, s.c_str(), N - 1);
    out[N - 1] = '\0';
  }
  return n;
}

inline size_t serializeJson(const JsonDocumentBase& doc, char* out, size_t capacity) {
  String s;
  size_t n = serializeJson(doc, s);
  if (out && capacity > 0) {
    std::strncpy(out, s.c_str(), capacity - 1);
    out[capacity - 1] = '\0';
  }
  return n;
}

template <typename T>
size_t serializeJson(const JsonDocumentBase& doc, T& out) { String s; size_t n = serializeJson(doc, s); out.print(s); return n; }

template <typename T>
size_t serializeJson(const JsonObject& obj, T& out) { String s; size_t n = serializeJson(obj, s); out.print(s); return n; }

template <typename T>
size_t serializeJson(const JsonArray& arr, T& out) { String s; size_t n = serializeJson(arr, s); out.print(s); return n; }
