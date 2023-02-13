#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

#define log Serial.println

using namespace std;

/* Put your SSID & Password */
const char* ssid = "NodeMCU";  // Enter SSID here
const char* password = "12345678";  //Enter Password here
const int sea_level_pressure = 1019;

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
AsyncWebServer server(80);

Adafruit_BMP3XX bmp;

double altitude, altitude_offset, altitude_max;

void setup() {
  ///////////////////////
  // SERIAL INIT
  Serial.begin(115200);

  ///////////////////////
  // BMP388 INIT
  log((bmp.begin_I2C()) ? "Successfully found valid BMP3 sensor!" : "Failed to find a valid BMP3 sensor");
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  ///////////////////////
  // WIFI AP INIT
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(200);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
    
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
    calibrateAltitude();
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/altitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", to_string(getAltitude()).c_str());
  });

  server.on("/altitude_max", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", to_string(getAltitudeMax()).c_str());
  });
  
  server.begin();
  log("HTTP server started");

  ///////////////////////
  // FS INIT
  log((SPIFFS.begin()) ? "Successfully mounted SPIFFS" : "Failed to mount SPIFFS");
  File file = SPIFFS.open("/text.txt", "r");
  log((file) ? "Succesfully opened file for reading" : "Failed to open file for reading");
  log();
  log("File Content");
  while(file.available()) Serial.write(file.read());
  file.close();

  ///////////////////////
  // DATA INIT
  altitude = 0;
  altitude_offset = 0;
  altitude_max = 0;
}

void loop() {
  if (! bmp.performReading()) {
    log("Failed to perform reading :(");
  }
  
  altitude = bmp.readAltitude(sea_level_pressure) - altitude_offset;
  if(altitude_max < altitude) altitude_max = altitude;

  log(altitude);
}

String processor(const String& var){
  if (var == "ALTITUDE"){
    return to_string(getAltitude()).c_str();
  }
  else if (var == "ALTITUDE_MAX"){
    return to_string(getAltitudeMax()).c_str();
  }
  return String();
}

double getAltitude(){
  return altitude;
}

double getAltitudeMax(){
  return altitude_max;
}

void calibrateAltitude(){
  altitude = 0;
  altitude_offset = bmp.readAltitude(sea_level_pressure);
  altitude_max = 0;
}
