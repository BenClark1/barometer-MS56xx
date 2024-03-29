//Ben Clark

//Barometer Library
#include "barometer_MS56xx.h"
#include <Wire.h> 
#include <math.h>

//Barometer Configuration
#define BARO_PRESSURE   (0x48)  //72 in decimal
#define BARO_TEMP               (0x58)  //88
#define BARO_ADDRESS    (0x77)  //119
#define BARO_RESET              (0x1E)  //30 
#define BARO_PROM_READ  (0xA2)  //162
#define BAROMETER_ID 4 

//calibration data
uint16_t C[7] = {0};

bool MS5611; //to specify what model barometer is in use

void initMS56xx(String barometer_model) { //for barometer_model, you MUST enter a string that is 
  //exactly "MS5611" or "MS5607", anything else will run the MS5607 math by default,
  //this parameter changes the math for the readPressure funtion
  if (barometer_model == "MS5611") {
    MS5611 = true;
  }
  else {
    MS5611 = false;
  }
  
  //reset the barometer
  Wire.beginTransmission(BARO_ADDRESS);
  Wire.write(BARO_RESET);
  Wire.endTransmission();
  delay(100);

  digitalWrite(13, LOW);

  for(uint8_t i = 0; i < 6; i++) {
    //write into PROM data
    Wire.beginTransmission(BARO_ADDRESS);
    Wire.write(BARO_PROM_READ + (i * 2)); 
    Wire.endTransmission();

    //read calibration data
    Wire.requestFrom(BARO_ADDRESS, (uint8_t) 2);
    while(Wire.available() == 0);

    digitalWrite(13, HIGH);

    C[i+1] = (Wire.read() << 8) | Wire.read();
    String help = "C" + String((i+1));
    help += ": ";
    help += String(C[i+1]);
    help += "/n";
    delay(10);
  }
  return;
} //end of init function

//Primes MS56xx to organize temperature data for reading
void primeTempMS56xx(void) {
  Wire.beginTransmission(BARO_ADDRESS);
  Wire.write(BARO_TEMP);
  Wire.endTransmission();
  return;
}

//Primes MS56xx to organize presssure data for reading
void primePressureMS56xx(void) {
  Wire.beginTransmission(BARO_ADDRESS);
  Wire.write(BARO_PRESSURE);
  Wire.endTransmission();
  return;
}

//Reads the raw values based on the previous priming method 
//Works for temperature or pressure
uint32_t readRawMS56xx(void) {
  Wire.beginTransmission(BARO_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(BARO_ADDRESS, 3);
  while(Wire.available() == 0);
  return(((uint32_t)Wire.read() << 16) | ((uint32_t)Wire.read() << 8) | (uint32_t)Wire.read());
}

//Reads MS56xx temperature data into MS5611 data struct
//Assumptions:
  //MS56xx primed for temperature
  //10ms delay after priming

void readTempMS56xx(struct MS56xx_packet *data) {
  uint32_t D2;
  int32_t T;
  int32_t dT;

  D2 = readRawMS56xx();
  dT = D2 - ((uint32_t)C[5] << 8);  //update '_dT'
  // Below, 'dT' and '_C[6]'' must be casted in order to prevent overflow
  // A bitwise division can not be done since it is unpredictible for signed integers
  T = 2000 + (((int64_t)dT * C[6])/8388608);
  data->temp = T/100.0;
  return;
}

//Function below: reads MS56xx pressure data into MS56xxdata struct
//Assumptions:
  //MS56xx is primed for pressure
  //10ms delay after priming

void readPressureMS56xx(struct MS56xx_packet *data) {
  int64_t OFF;
  int64_t SENS;
  uint32_t D1 = readRawMS56xx(); 
  int32_t dT = (int32_t)(((((data->temp) * 100.0) - 2000) * 8388608) / C[6]);
  
  if (MS5611) { //this is the math that will work with the MS5611
    OFF = ((int64_t)C[2])*65536 + ((int64_t)C[4]*dT)/128; 
    SENS = ((int64_t)C[1])*32768 + ((int64_t)C[3]*dT)/256;
  }
  else { //this is the math that works with the MS5607
    OFF = ((int64_t)C[2])*131072 + ((int64_t)C[4]*dT)/64;
    SENS = ((int64_t)C[1])*65536 + ((int64_t)C[3]*dT)/128;
  }

  int32_t P = (D1*(SENS/2097152) - OFF)/32768;

  data->pressure = P / 100.0;
  return;
}

//Calculate altitude using data from MS56xxdata struct
void calcAltitudeMS56xx(MS56xx_packet *data) {
  data->altitude = ((pow((1013.25 / data->pressure), 1/5.275) - 1.0) * (data->temp + 273.15))/0.0065;
}

//Convert MS56xx struct data to string for output
String MS56xxToString(struct MS56xx_packet *data) { 
  String out = "Pressure = " + String(data->pressure);
  out += "\nTemperature = " + String(data->temp);
  out += "\nAltitude = " + String(data->altitude);
  out += "\n";
  return out;
}
