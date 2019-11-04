#include "BMA250.h"       // For interfacing with the accel. sensor

// Accelerometer sensor variables for the sensor and its values
BMA250 accel_sensor;

void setup_accel() {
  SerialMonitorInterface.print("Initializing BMA...");
  // Set up the BMA250 acccelerometer sensor
  accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms); 
}


// Write to outputData
boolean loop_accel(struct AccelData *outputData) {
  accel_sensor.read();//This function gets new data from the acccelerometer
  boolean empty = sensor_empty(accel_sensor.X, accel_sensor.Y, accel_sensor.Z);

  int temp = ((accel_sensor.rawTemp * 0.5) + 24.0);
  outputData->x = accel_sensor.X;
  outputData->y = accel_sensor.Y;
  outputData->z = accel_sensor.Z;
  outputData->temp = temp;

  return !empty;
}

bool sensor_empty(int x, int y, int z) {
  // If the BMA250 is not found, nor connected correctly, these values will be produced
  // by the sensor 
  return x == -1 && y == -1 && z == -1;
}

// Prints the sensor values to the Serial Monitor, or Serial Plotter (found under 'Tools')
// read-only
void showSerialAccel(struct AccelData *data) {
  SerialMonitorInterface.print("X = ");
  SerialMonitorInterface.print(data->x);
  
  SerialMonitorInterface.print("  Y = ");
  SerialMonitorInterface.print(data->y);
  
  SerialMonitorInterface.print("  Z = ");
  SerialMonitorInterface.print(data->z);

  SerialMonitorInterface.print("  Temperature(C) = ");
  SerialMonitorInterface.println(data->temp);
}
