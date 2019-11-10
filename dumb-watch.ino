/*************************************************************************
 * BMA250 Tutorial:
 * This example program will show the basic method of printing out the 
 * accelerometer values from the BMA250 to the Serial Monitor, and the
 * Serial Plotter
 * 
 * Hardware by: TinyCircuits
 * Code by: Laverena Wienclaw for TinyCircuits
 *
 * Initiated: Mon. 11/1/2018
 * Updated: Tue. 11/2/2018
 ************************************************************************/
 
#include <Wire.h>         // For I2C communication with sensor
#include <ArduinoLowPower.h>
#include "accel.h"

#if defined(ARDUINO_ARCH_SAMD)
 #define SerialMonitorInterface SerialUSB
#else
 #define SerialMonitorInterface Serial
#endif

void setup() {
  SerialMonitorInterface.begin(115200);
  Wire.begin();
  //setup_accel();
  setup_display();
}



struct AccelData inputData;
struct AccelResult accel_result;

void loop() {
/*
  // The BMA250 can only poll new sensor values every 64ms, so this delay
  boolean success = loop_accel(&inputData);
  if (!success) {
    SerialMonitorInterface.print("ERROR! NO BMA250 DETECTED!");
  } else {
    showSerialAccel(&inputData);
  }
  accel_result.data = inputData;
  accel_result.success = success;
  */
  accel_result.success = false;

  loop_display(&accel_result);

  // To see the terminal output, don't do deep sleep
  delay(50);
  // LowPower.deepSleep(50);
}