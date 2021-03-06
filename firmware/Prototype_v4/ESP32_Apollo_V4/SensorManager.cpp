/*
 *   ESP32 Oxygen Concentrator
 *  ===========================
 * 
 * This software is provided "as is" for educational purposes only. 
 * No claims are made regarding its fitness for medical or any other purposes. 
 * The authors are not liable for any injuries, damages or financial losses.
 * 
 * Use at your own risk!
 * 
 * License: MIT https://github.com/oxycon/ProjectApollo/blob/master/LICENSE.txt
 * For more information see: https://github.com/oxycon/ProjectApollo
 */

#include <Arduino.h>

#include "Hardware.h"
#include "Config.h"
#include "Error.h"
#include "SensorManager.h"

Bme bme280_1;
Bme bme280_2;
Shtc3 shtc3;
Hdc2080 hdc2080_1;
Hdc2080 hdc2080_2;
Htu21d htu21d;
Mprls mprls;
Tcs34725 tcs34725;

Sensor* in_pressure_sensor = nullptr;
Sensor* out_pressure_sensor = nullptr;
Sensor* ambient_sensor = nullptr;
Sensor* intake_sensor = nullptr;
Sensor* desiccant_sensor = nullptr;
Sensor* output_sensor = nullptr;
Tcs34725* color_sensor = nullptr;

Sensor* installed_sensors[10];
size_t installed_sensor_count = 0;

static uint32_t next_sensor_read_ms_ = 0;
const uint32_t sensor_delay_ms = 100;

Sensor* find_sensor(const char* name, uint16_t address, ErrorType err) {
  if (address == 0xFFFF) return nullptr;
  Sensor* result = nullptr;
  if (bme280_1.isFound() && address == bme280_1.getAddress()) { result = &bme280_1; }
  else if (bme280_2.isFound() && address == bme280_2.getAddress()) { result = &bme280_2; }
  else if (shtc3.isFound() && address == shtc3.getAddress()) { result = &shtc3; }
  else if (hdc2080_1.isFound() && address == hdc2080_1.getAddress()) { result = &hdc2080_1; }
  else if (hdc2080_2.isFound() && address == hdc2080_2.getAddress()) { result = &hdc2080_2; }
  else if (htu21d.isFound() && address == htu21d.getAddress()) { result = &htu21d; }
  if (result) {
     DEBUG_printf(FS("Found %s sensor: %s at %0X\n"), name, result->getTypeName(), address);
     result->name = name;
     installed_sensors[installed_sensor_count++] = result;
  } else {
     DEBUG_printf(FS("Could not find %s sensor at %0X\n"), name, address);    
     setError(err);
  }
  return result;
}

void sensor_setup() {
  bme280_1.begin(BME280_ADDRESS);
  bme280_2.begin(BME280_ADDRESS_ALTERNATE); 
  shtc3.begin();
  hdc2080_1.begin(HDC2080_ADDRESS_1);
  hdc2080_2.begin(HDC2080_ADDRESS_2);
  htu21d.begin();
  mprls.begin();
  tcs34725.begin();

  ambient_sensor = find_sensor(FS("ambient"), config.concentrator.ambient_sensor_address, AMBIENT_HUMIDITY_SENSOR_NOT_FOUND);
  intake_sensor = find_sensor(FS("intake"), config.concentrator.intake_sensor_address, INTAKE_HUMIDITY_SENSOR_NOT_FOUND);
  desiccant_sensor = find_sensor(FS("desiccant"), config.concentrator.desiccant_sensor_address, DESICCANT_HUMIDITY_SENSOR_NOT_FOUND);
  output_sensor = find_sensor(FS("output"), config.concentrator.output_sensor_address, OUTPUT_HUMIDITY_SENSOR_NOT_FOUND);
  if (config.concentrator.out_pressure_address != 0xFFFF) {
    if (mprls.isFound()) {
      mprls.name = "out-pressure";
      out_pressure_sensor = &mprls;
      installed_sensors[installed_sensor_count++] = &mprls;
    } else {
       setError(OUT_PRESSURE_SENSOR_NOT_FOUND);    
    }
  }
  if (config.concentrator.color_sensor_address != 0xFFFF) {
    if (tcs34725.isFound()) {
      color_sensor =  &tcs34725;
      color_sensor->name = "color";
    } else {
       setError(COLOR_SENSOR_NOT_FOUND);
    }
  }
  
  next_sensor_read_ms_ = millis();
}

void sensor_run() {
 if (millis() < next_sensor_read_ms_) { return; }
 next_sensor_read_ms_ += sensor_delay_ms;
 if (ambient_sensor) { ambient_sensor->run(); }
 if (intake_sensor) { intake_sensor->run(); }
 if (desiccant_sensor) { desiccant_sensor->run(); }
 if (output_sensor) { output_sensor->run(); }
 if (in_pressure_sensor) { in_pressure_sensor->run(); }
 if (out_pressure_sensor) { out_pressure_sensor->run(); }
 if (color_sensor) { color_sensor->run(); }
}
