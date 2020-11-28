#include <Adafruit_ESP8266.h>
#include <SoftwareSerial.h>

#ifndef ESP_RX
#define ESP_RX   8
#endif

#ifndef ESP_TX
#define ESP_TX   7
#endif

#ifndef ESP_RST
#define ESP_RST  4
#endif

SoftwareSerial softser(ESP_RX, ESP_TX);

#ifdef ESP_DEBUG
Adafruit_ESP8266 wifi(&softser, &Serial, ESP_RST);
#else
Adafruit_ESP8266 wifi(&softser, NULL, ESP_RST);
#endif

#define GET_METHOD "GET"

char buffer[100];

boolean esp8266_setup() {

  // comment/replace this if you are using something other than v 0.9.2.4!
  wifi.setBootMarker(F("Version:0.9.2.4]\r\n\r\nready"));

  softser.begin(9600); // Soft serial connection to ESP8266

  Serial.print(F("Hard reset..."));
//  if(!wifi.hardReset()) {
//    Serial.println(F("no response from module."));
//    return false;
//  }
  Serial.println(F("OK."));

  Serial.print(F("Soft reset..."));
  if(!wifi.softReset()) {
    Serial.println(F("no response from module."));
    // return false;
  }
  Serial.println(F("OK."));

  Serial.print(F("Checking firmware version..."));
  wifi.println(F("AT+GMR"));
  if(wifi.readLine(buffer, sizeof(buffer))) {
    Serial.println(buffer);
    wifi.find(); // Discard the 'OK' that follows
    return true;
  } 
  else {
    Serial.println(F("error"));
    return false;
  }
}

boolean esp8266_connectAP(Fstr* ssid, Fstr* pass) {
  Serial.print(F("Connecting to WiFi..."));
  //  if(wifi.connectToAP(F(ESP_SSID), F(ESP_PASS))) {
  if(wifi.connectToAP(ssid, pass)) {

    // IP addr check isn't part of library yet, but
    // we can manually request and place in a string.
    Serial.print(F("OK\nChecking IP addr..."));
    wifi.println(F("AT+CIFSR"));
    if(wifi.readLine(buffer, sizeof(buffer))) {
      Serial.println(buffer);
      wifi.find(); // Discard the 'OK' that follows
      return true;
    } 
    else { // IP addr check failed
      Serial.println(F("error"));
      return false;
    }
    wifi.closeAP();
  } 
  else { // WiFi connection failed
    Serial.println(F("FAIL"));
    return false;
  }
}

void esp8266_closeAP() {
  wifi.closeAP();  
}

boolean esp8266_connectTCP(Fstr *host, int port) {
  int try_count = 10;
  while(try_count > 0) {
    if(wifi.connectTCP(host, port)) {
      return true;
    }
    delay(100);
  }
  return false;
}


boolean esp8266_requestURL(char *page, char* method) {
  return wifi.requestURL(page, method);
}

boolean esp8266_requestURL(char *page) {
  return wifi.requestURL(page, GET_METHOD);
}

void esp8266_closeTCP() {
  wifi.closeTCP();
}

int esp8266_readLine(char *buf, int bufSiz) {
  return wifi.readLine(buf, bufSiz);
}

int esp8266_available() {
  return wifi.available();
}

char esp8266_read() {
  return wifi.read();
}

boolean esp8266_find(Fstr *str, boolean ipd = false) {
  return wifi.find(str, ipd);
}

boolean esp8266_find() {
  return wifi.find();
}

