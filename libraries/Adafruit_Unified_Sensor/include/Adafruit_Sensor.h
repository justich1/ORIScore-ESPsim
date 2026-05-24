#pragma once

#include "Arduino.h"
#include <cstring>

#define SENSORS_GRAVITY_STANDARD (9.80665F)
#define SENSORS_PRESSURE_SEALEVELHPA (1013.25F)

typedef enum {
  SENSOR_TYPE_ACCELEROMETER = 1,
  SENSOR_TYPE_MAGNETIC_FIELD = 2,
  SENSOR_TYPE_ORIENTATION = 3,
  SENSOR_TYPE_GYROSCOPE = 4,
  SENSOR_TYPE_LIGHT = 5,
  SENSOR_TYPE_PRESSURE = 6,
  SENSOR_TYPE_PROXIMITY = 8,
  SENSOR_TYPE_GRAVITY = 9,
  SENSOR_TYPE_LINEAR_ACCELERATION = 10,
  SENSOR_TYPE_ROTATION_VECTOR = 11,
  SENSOR_TYPE_RELATIVE_HUMIDITY = 12,
  SENSOR_TYPE_AMBIENT_TEMPERATURE = 13,
  SENSOR_TYPE_VOLTAGE = 15,
  SENSOR_TYPE_CURRENT = 16,
  SENSOR_TYPE_COLOR = 17
} sensors_type_t;

typedef struct {
  int32_t version;
  int32_t sensor_id;
  int32_t type;
  int32_t reserved0;
  int32_t timestamp;
  union {
    float data[4];
    struct { float x; float y; float z; };
    struct { float temperature; };
    struct { float distance; };
    struct { float light; };
    struct { float pressure; };
    struct { float relative_humidity; };
    struct { float current; };
    struct { float voltage; };
  };
} sensors_event_t;

typedef struct sensor_s {
  char name[12];
  int32_t version;
  int32_t sensor_id;
  int32_t type;
  float max_value;
  float min_value;
  float resolution;
  int32_t min_delay;
} sensor_t;

class Adafruit_Sensor {
public:
  virtual ~Adafruit_Sensor() = default;
  virtual void enableAutoRange(bool enabled) { (void)enabled; }
  virtual bool getEvent(sensors_event_t*) { return false; }
  virtual void getSensor(sensor_t*) {}
};
