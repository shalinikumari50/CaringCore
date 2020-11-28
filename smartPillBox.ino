#include <MPR121.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>
#include <DHT_U.h>
#define BAUD_RATE 9600
#define numElectrodes 6
#define INTERUPT_PIN D6
#define DHTPIN            D4
#define LED0 D8
#define LED1 D7
#define LED2 D5
#define LED3 D3
#define LED4 D0
#define DHTTYPE           DHT22      // DHT 22 (AM2302)
int ledPins[] = {LED0, LED1, LED2, LED3, LED4};
#define HOUR 3600000
//------- i/o classes ---------
DHT_Unified dht(DHTPIN, DHTTYPE);
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
//WIFI configuration
#define WLAN_SSID       ""
#define WLAN_PASS       ""

// Functions
void MQTTconnect();
void WifiConnect();
void dhtSetup(sensor_t sensor);
void setupMRP121();
void sendTemperatureEvent();
void sendHumidityEvent();
void sendHumidityEvent();
int parseDay(uint8_t* day);
void listenOnLedMQTT();
void checkForTouch(int i);
void blinkAndToggleLight(int i);


//MQTT SERVER CONFIG
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME ""
#define AIO_KEY ""

// Store the MQTT server, client ID, username, and password in flash memory.
const char MQTT_SERVER[] = AIO_SERVER;
const int  MQTT_PORT = AIO_SERVERPORT;

// Set a unique MQTT client ID using the AIO key + the date and time the sketch
// was compiled (so this should be unique across multiple devices for a user,
// alternatively you can manually set this to a GUID or other random value).
const char MQTT_CLIENTID[] = AIO_KEY __DATE__ __TIME__;
const char MQTT_USERNAME[] = AIO_USERNAME;
const char MQTT_PASSWORD[] = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// Setup MQTT feeds
const char TEMPERATURE_FEED[] = AIO_USERNAME "/feeds/smartmed.temperature";
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE_FEED);
#define TEMP_LIMIT 30.0
const char HUMIDITY_FEED[] = AIO_USERNAME "/feeds/smartmed.humidity";
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, HUMIDITY_FEED);
#define HUMIDITY_LIMIT 50
const char FORGOT_FEED[] = AIO_USERNAME "/feeds/smartmed.forgot";
Adafruit_MQTT_Publish forgot = Adafruit_MQTT_Publish(&mqtt, FORGOT_FEED);
const char LED_FEED[] = AIO_USERNAME "/feeds/smartmed.led";
Adafruit_MQTT_Subscribe turnOnLed = Adafruit_MQTT_Subscribe(&mqtt, LED_FEED);


//global vars
int currentDay = 0;
boolean isLedOn = false;
unsigned long lastPubTime = 0;
unsigned long pillTime = 0;
boolean sentForgotMsg = true;

void setup() {
  Serial.begin(BAUD_RATE);
  setupMRP121();
  //DHT Sensor setup
  sensor_t sensor;
  dhtSetup(&sensor);
  dht.begin();
  WifiConnect();
  MQTTconnect();

  // sets the digital pin as output.
  for (int i = 0 ; i < 5; i ++) {
    pinMode(ledPins[i], OUTPUT);
  }

}

void loop() {

  if (isLedOn) {
    checkForTouch(currentDay);

    // send sms to tell the user he forgot to take his pills
    if (!sentForgotMsg) {
      if (millis() - pillTime >= HOUR) {
        if (!forgot.publish("send")) {
          Serial.println("Failed to publish forgot");
          sentForgotMsg = true;
        }
        else
          Serial.println("Forgot SMS was published successfully!");
      }
    }

  }


  if (!lastPubTime) {
    lastPubTime = millis();
  }
  //publish and listen to mqtt every 30 secs
  if (millis() - lastPubTime > 30000) {
    lastPubTime = millis();
    listenOnLedMQTT();
    sendTemperatureEvent();
    sendHumidityEvent();
  }

  delay(10);
  yield();
}


void checkForTouch(int i) {
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();
    if (MPR121.isNewTouch(i)) {
      blinkAndToggleLight(i);
    }
  }
}

void listenOnLedMQTT() {
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(3000))) {
    if (subscription == &turnOnLed) {
      currentDay = parseDay(turnOnLed.lastread);
      yield();
      blinkAndToggleLight(currentDay);
      pillTime = millis();
      sentForgotMsg = false;
    }
  }
}

void WifiConnect() {
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  // wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    yield();
    Serial.print(".");
    delay(10);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  yield();
  // connect to adafruit io
  Serial.println(WiFi.localIP());
  Serial.println();
}

void dhtSetup(sensor_t* sensor) {
  dht.temperature().getSensor(sensor);
  dht.humidity().getSensor(sensor);

  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor -> name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor -> version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor -> sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor -> max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor -> min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor -> resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
}

// connect to adafruit io via MQTT
void MQTTconnect() {
  mqtt.subscribe(&turnOnLed);
  Serial.print("Connecting to MQTT Server... ");
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println("Wrong protocol"); break;
      case 2: Serial.println("ID rejected"); break;
      case 3: Serial.println("Server unavail"); break;
      case 4: Serial.println("Bad user/pass"); break;
      case 5: Serial.println("Not authed"); break;
      case 6: Serial.println("Failed to subscribe"); break;
      default: Serial.println("Connection failed"); break;
    }
    if (ret >= 0) {
      mqtt.disconnect();
    }
    Serial.println("Retrying connection...");
    delay(5000);
  }
  Serial.println("Connected!");


}

void sendTemperatureEvent() {
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    Serial.println(" *C");

    if (event.temperature > TEMP_LIMIT) {
      //Publish temperature data
      if (! temperature.publish(event.temperature))
        Serial.println("Failed to publish temperature");
      else
        Serial.println("Temperature was published successfully!");

    }
  }
}


void blinkAndToggleLight(int day) {
  if (!isLedOn) {
    for (int i = 0 ; i < 7 ; i ++) {
      //turnOff Any Other Led
      digitalWrite(ledPins[i], LOW );
    }
  }
  for (int i = 0 ; i < 4 ; i ++) {
    digitalWrite(ledPins[day], LOW);
    delay(500);
    digitalWrite(ledPins[day], HIGH);
    delay(500);
  }
  isLedOn = !isLedOn;
  digitalWrite(ledPins[day], isLedOn);

}


int parseDay(uint8_t*  day) {
  if (0 == strcmp((char *) day, "0"))
    return 0;
  if (0 == strcmp((char *) day, "1"))
    return 1;
  if (0 == strcmp((char *)day, "2"))
    return 2;
  if (0 == strcmp((char *) day, "3"))
    return 3;
  if (0 == strcmp((char *) day, "4"))
    return 4;
  if (0 == strcmp((char *)day, "5"))
    return 5;
  if (0 == strcmp((char *)day, "6"))
    return 6;
  else
    return 7;

}

void sendHumidityEvent() {
  // Get humidity event and print its value.
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    float relative_humidity = event.relative_humidity ;
    Serial.print("Humidity: ");
    Serial.print(relative_humidity);
    Serial.println("%");
    if (relative_humidity > HUMIDITY_LIMIT) {
      //Publish humidity data
      if (! humidity.publish(relative_humidity))
        Serial.println("Failed to publish humidity");
      else
        Serial.println("Humidity was published successfully!");

    }
  }
}

void setupMRP121() {

  Wire.begin();

  // 0x5A is the MPR121 I2C address on the Bare Touch Board
  if (!MPR121.begin(0x5A)) {
    Serial.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;
    }
    while (1);
  }

  // pin D6 is the MPR121 interrupt
  MPR121.setInterruptPin(INTERUPT_PIN);

  // this is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch
  MPR121.setTouchThreshold(40);

  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  // default value is 20 for touch
  MPR121.setReleaseThreshold(20);

  // initial data update
  MPR121.updateTouchData();
}



