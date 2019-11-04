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
#include "accel.h"

#if defined(ARDUINO_ARCH_SAMD)
 #define SerialMonitorInterface SerialUSB
#else
 #define SerialMonitorInterface Serial
#endif

void setup() {
  SerialMonitorInterface.begin(115200);
  Wire.begin();
  setup_accel();
  setup_display();
}



struct AccelData inputData;
struct AccelResult accel_result;

void loop() {
  boolean success = loop_accel(&inputData);
  if (!success) {
    SerialMonitorInterface.print("ERROR! NO BMA250 DETECTED!");
  } else {
    showSerialAccel(&inputData);
  }

  accel_result.success = success;
  accel_result.data = inputData;
  loop_display(&accel_result);

  // The BMA250 can only poll new sensor values every 64ms, so this delay
  // will ensure that we can continue to read values
  delay(250);
  // ***Without the delay, there would not be any sensor output*** 
}
