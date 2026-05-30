#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

#ifndef BUFFER_LENGTH
#define BUFFER_LENGTH 32
#endif

#ifndef I2C_BUFFER_LENGTH
#define I2C_BUFFER_LENGTH 32
#endif

class TwoWire {
public:
    TwoWire() {}
    explicit TwoWire(uint8_t bus_num) {
        (void)bus_num;
    }

    void begin() {}

    bool begin(int sda, int scl) {
        (void)sda;
        (void)scl;
        return true;
    }

    bool begin(int sda, int scl, uint32_t frequency) {
        (void)sda;
        (void)scl;
        (void)frequency;
        return true;
    }

    void end() {}

    void setClock(uint32_t frequency) {
        (void)frequency;
    }

    void setTimeOut(uint16_t timeOutMillis) {
        (void)timeOutMillis;
    }

    void beginTransmission(uint8_t address) {
        (void)address;
    }

    void beginTransmission(int address) {
        beginTransmission((uint8_t)address);
    }

    uint8_t endTransmission(void) {
        return 0;
    }

    uint8_t endTransmission(bool stopBit) {
        (void)stopBit;
        return 0;
    }

    size_t write(uint8_t data) {
        (void)data;
        return 1;
    }

    size_t write(int data) {
        return write((uint8_t)data);
    }

    size_t write(const uint8_t* data, size_t quantity) {
        (void)data;
        return quantity;
    }

    size_t write(const char* data) {
        if (!data) return 0;

        size_t len = 0;
        while (data[len] != '\0') len++;
        return len;
    }

    int requestFrom(uint8_t address, uint8_t quantity) {
        (void)address;
        return quantity;
    }

    int requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
        (void)address;
        (void)sendStop;
        return quantity;
    }

    int requestFrom(uint8_t address, size_t quantity, bool sendStop) {
        (void)address;
        (void)sendStop;
        return (int)quantity;
    }

    int requestFrom(int address, int quantity) {
        return requestFrom((uint8_t)address, (uint8_t)quantity);
    }

    int requestFrom(int address, int quantity, int sendStop) {
        return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
    }

    int available(void) {
        return 0;
    }

    int read(void) {
        return -1;
    }

    int peek(void) {
        return -1;
    }

    void flush(void) {}

    void onReceive(void (*function)(int)) {
        (void)function;
    }

    void onRequest(void (*function)(void)) {
        (void)function;
    }
};

extern TwoWire Wire;
extern TwoWire Wire1;