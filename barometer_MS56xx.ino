//Ben Clark

//Barometer Library Testing

#include "barometer_MS56xx.h"
#include <Wire.h>
#include <math.h>

struct MS56xx_packet bar;

void setup() {
  Serial.begin(9600); //must change this to SerialUSB.begin() if using a MKR ZERO
  Wire.begin();
}

void loop() {
  initMS56xx("MS5607"); //call the init function with the specified barometer model

  primeTempMS56xx();
  delay(10); //must delay 10 milliseconds after priming

  readTempMS56xx(&bar);

  primePressureMS56xx();
  delay(10);

  readPressureMS56xx(&bar);

  calcAltitudeMS56xx(&bar);
  
  String output = MS56xxToString(&bar);
  Serial.println(output);
  delay(2000);
}
