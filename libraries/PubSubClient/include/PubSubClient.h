#pragma once

#include "Arduino.h"
#include "WiFi.h"
#include <functional>
#include <vector>

#ifndef MQTT_CONNECTED
#define MQTT_CONNECTED 0
#endif

class PubSubClient {
public:
  using Callback = void (*)(char*, byte*, unsigned int);

  PubSubClient() = default;
  explicit PubSubClient(WiFiClient& client) : _client(&client) {}

  PubSubClient& setClient(WiFiClient& client) {
    _client = &client;
    return *this;
  }

  PubSubClient& setServer(const char* domain, uint16_t port) {
    _server = domain ? domain : "";
    _port = port;
    Serial.println(String("[MQTT SIM] server=") + _server + ":" + String((int)_port));
    return *this;
  }

  PubSubClient& setCallback(Callback cb) {
    _callback = cb;
    return *this;
  }

  PubSubClient& setBufferSize(uint16_t size) {
    _bufferSize = size;
    Serial.println(String("[MQTT SIM] buffer=") + String((int)_bufferSize));
    return *this;
  }

  boolean connect(const char* id) {
    return connect(id, nullptr, nullptr);
  }

  boolean connect(const char* id, const char* user, const char* pass) {
    _clientId = id ? id : "";
    _connected = true;
    _state = MQTT_CONNECTED;

    Serial.println(String("[MQTT SIM] connect id=") + _clientId +
                   " user=" + String(user ? user : "") +
                   " server=" + _server + ":" + String((int)_port));

    (void)pass;
    return true;
  }

  boolean connected() {
    return _connected;
  }

  void disconnect() {
    if (_connected) Serial.println("[MQTT SIM] disconnect");
    _connected = false;
    _state = -1;
  }

  boolean loop() {
    if (!_connected) return false;

    if (_callback && !_injectedTopic.isEmpty()) {
      String topic = _injectedTopic;
      String payload = _injectedPayload;
      _injectedTopic = "";
      _injectedPayload = "";

      std::vector<byte> bytes;
      for (size_t i = 0; i < payload.length(); i++) bytes.push_back((byte)payload[i]);

      std::vector<char> topicBuf(topic.v.begin(), topic.v.end());
      topicBuf.push_back('\0');

      _callback(topicBuf.data(), bytes.data(), (unsigned int)bytes.size());
    }

    return true;
  }

  boolean publish(const char* topic, const char* payload) {
    return publish(topic, (const uint8_t*)(payload ? payload : ""), payload ? (unsigned int)std::strlen(payload) : 0);
  }

  boolean publish(const char* topic, const char* payload, boolean retained) {
    (void)retained;
    return publish(topic, payload);
  }

  boolean publish(const char* topic, const char* payload, unsigned int length) {
    return publish(topic, (const uint8_t*)payload, length);
  }

  boolean publish(const char* topic, const char* payload, size_t length) {
    return publish(topic, (const uint8_t*)payload, (unsigned int)length);
  }

  boolean publish(const char* topic, const uint8_t* payload, size_t length) {
    return publish(topic, payload, (unsigned int)length);
  }

  boolean publish(const char* topic, const uint8_t* payload, unsigned int length) {
    String msg;
    if (payload) {
      for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    }

    Serial.println(String("[MQTT SIM] publish topic=") + String(topic ? topic : "") + " payload=" + msg);
    return true;
  }

  boolean subscribe(const char* topic) {
    Serial.println(String("[MQTT SIM] subscribe ") + String(topic ? topic : ""));
    return true;
  }

  int state() {
    return _state;
  }

  // Pomocná simulace příchozí MQTT zprávy.
  // Zavolej třeba z testu/later helperu: client.simInjectMessage("topic", "{\"target_temperature\":22}");
  void simInjectMessage(const String& topic, const String& payload) {
    _injectedTopic = topic;
    _injectedPayload = payload;
  }

private:
  WiFiClient* _client = nullptr;
  Callback _callback = nullptr;
  String _server = "";
  uint16_t _port = 1883;
  uint16_t _bufferSize = 256;
  bool _connected = false;
  int _state = -1;
  String _clientId = "";
  String _injectedTopic = "";
  String _injectedPayload = "";
};
