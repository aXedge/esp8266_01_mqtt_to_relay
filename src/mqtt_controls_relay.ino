#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <stdarg.h>
//#include <dummy.h>

#if 0
//ref: https://circuits4you.com/2017/12/31/nodemcu-pinout/
#define PIN_WIRE_SDA                (4)
#define PIN_WIRE_SCL                (5)

static const uint8_t SDA            = PIN_WIRE_SDA;
static const uint8_t SCL            = PIN_WIRE_SCL;

static const uint8_t LED_BUILTIN    = 16;
static const uint8_t BUILTIN_LED    = 16;
//          NodeMCU Pin,    GPIO (Ardu),  PCB Pin Name
static const uint8_t D0             = 16;   // WAKE_UP
static const uint8_t D1             = 5;
static const uint8_t D2             = 4;
static const uint8_t D3             = 0;    // FLASH
static const uint8_t D4             = 2;    // TXD1
static const uint8_t D5             = 14;
static const uint8_t D6             = 12;
static const uint8_t D7             = 13;   // RXD2
static const uint8_t D8             = 15;   // TXD2
static const uint8_t D9_RX          = 3;    // RXD0
static const uint8_t D10_TX         = 1;    // TXD0
static const uint8_t A_IN           = A0;   // Analog input pin that the potentiometer is attached to
static const uint8_t A_OUT          = 9;    // SD2
#endif /* if 0 */

// the onoffbutton feed sets the PWM output of this pin
#define DIGITAL_PIN                 2     // GPIO2/D4
#define SERIAL_PRINTF_MAX_BUFF      256

/************************* WiFi Access Point *********************************/

#define WLAN_SSID                   "__UPDATE_ME__"
#define WLAN_PASS                   "__UPDATE_ME__"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER                  "io.adafruit.com"
#define AIO_SERVERPORT              1883                // use 8883 for SSL
#define AIO_USERNAME                "__UPDATE_ME__"     //"...your AIO username (see https://accounts.adafruit.com)..."
#define AIO_KEY                     "__UPDATE_ME__"     //"...your AIO key..."
#define AIO_MQTT_FEED_ONOFFBUTTON   "__UPDATE_ME__"     //"...your AIO feed to use for communicating over MQTT..."

unsigned int state                  = LOW;              // Variable for the state of the LED
uint32_t delayMS_dht                = 2000;
uint32_t delayMS_loop               = 1000;
uint32_t delayMS_mqtt               = 1000;
unsigned int count                  = 0;
bool exit_loop                      = false;
bool skip_loop                      = false;
boolean stringComplete              = false;            // whether the string is complete
String inputString                  = "";               // a string to hold incoming data

// https://medium.com/@kslooi/print-formatted-data-in-arduino-serial-aaea9ca840e3
char buff[SERIAL_PRINTF_MAX_BUFF]   = {0};

// Arduino JSON: https://arduinojson.org/v6/api/json/serializejson/
DynamicJsonDocument doc(1024);  // Keeps the 'doc' on Heap
//StaticJsonDocument<1024> doc; // Keeps the 'doc' on Stack

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// Setup a feed called 'arb_packet' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
#define ARB_FEED "aXedge/feeds/aio_packet"
Adafruit_MQTT_Publish ap = Adafruit_MQTT_Publish(&mqtt, ARB_FEED);

//ref: https://io.adafruit.com/api/docs/mqtt.html#adafruit-io-mqtt-api

//// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME AIO_MQTT_FEED_ONOFFBUTTON);
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME AIO_MQTT_FEED_ONOFFBUTTON, MQTT_QOS_1);

/*************************** Sketch Code ************************************/

/*
** ============================================================================
** Forward Declarations
** ============================================================================
**/
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
void onoffcallback(char *data, uint16_t len);
int report_dht_sensor_details();
void serialPrintf(const char *fmt, ...);
int handle_serial_event(String event_str);

/*
** ============================================================================
** Function Definitions
** ============================================================================
**/
/*---------------------------------------------------------------------------*/
void setup() {
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
  Serial.begin(115200);         // Set the communication speed
  pinMode(DIGITAL_PIN, OUTPUT);     // Initialize the GPIO2/D4 pin as an output
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.println(F("------------------------------------"));
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  Serial.println(F("------------------------------------"));

  // Setup MQTT subscription for onoff & slider feed.
  Serial.println(F("------------------------------------"));
  Serial.println(F("Subscribing to MQTT..."));
  onoffbutton.setCallback(onoffcallback);
  mqtt.subscribe(&onoffbutton);
  Serial.println(F("MQTT Done..."));
  Serial.println(F("------------------------------------"));
}

/*---------------------------------------------------------------------------*/
// the loop function runs over and over again forever
void loop() {
  delay(delayMS_loop);                      // Wait for a second
  if (!skip_loop) {
    digitalWrite(DIGITAL_PIN /* LED_BUILTIN */, state);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    serialPrintf("[%04d]: LED state: %s\n", count, state?"ON":"OFF");
//    state = state == LOW ? HIGH : LOW;  // Change state LOW <-> HIGH
    report_dht_sensor_details();

    {
      // Ensure the connection to the MQTT server is alive (this will make the first
      // connection and automatically reconnect when disconnected).  See the MQTT_connect
      // function definition further below.
      MQTT_connect();
      // this is our 'wait for incoming subscription packets and callback em' busy subloop
      // try to spend your time here:
      mqtt.processPackets(delayMS_mqtt);

      // ping the server to keep the mqtt connection alive
      // NOT required if you are publishing once every KEEPALIVE seconds
      /*
      if(! mqtt.ping()) {
        mqtt.disconnect();
      }
      */
    }
    count++;
  }

  if (Serial.available()) {
    serialEvent();
  }

  if (stringComplete) {
    handle_serial_event(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  if (exit_loop) {
    exit(0);
  }
}

/*---------------------------------------------------------------------------*/
/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
 // ref: https://www.arduino.cc/reference/en/language/functions/communication/serial/serialevent/
void serialEvent() {
  serialPrintf("Serial Rx Available: %d\n", Serial.available());
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    if ((inChar != '\n') && (inChar != '\r')) {
      inputString += inChar;
    }
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
    else {
//      serialPrintf("Rx Buffer: %s\n", inputString.c_str());
    }
  }
}

/*---------------------------------------------------------------------------*/
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print(F("Connecting to MQTT... "));

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println(F("Retrying MQTT connection in 5 seconds..."));
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println(F("MQTT Connected!"));
}

/*---------------------------------------------------------------------------*/
void onoffcallback(char *data, uint16_t len) {
  Serial.print("onoff callback, the received message is: ");
  Serial.println(data);
  deserializeJson(doc, data);
  if (doc["cmd"] == "turn_off") {
    state = LOW;
  } else {
    state = HIGH;
  }
  digitalWrite(LED_BUILTIN, state);   // Turn the LED on (Note that LOW is the voltage level
  digitalWrite(DIGITAL_PIN, !state);   // Turn the LED on (Note that LOW is the voltage level
  doc.remove("cmd");
  doc.remove("dest");
  // Publish latest DHT Sensor readings in response
  {
    size_t packet_len = 0;
    report_dht_sensor_details();
    buff[0] = 0;
    packet_len = serializeJson(doc, buff, SERIAL_PRINTF_MAX_BUFF);
    if (packet_len > 0) {
      if (! ap.publish((uint8_t *)buff, packet_len))
        Serial.println(F("Publish Failed."));
      else {
        Serial.println(F("Publish Success!"));
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
int report_dht_sensor_details() {
  long outputValue = 0;
  sensors_event_t event;

  doc["sensor"] = "dht11";
  doc["time"]   = count;
  doc["readings"]["temp"] = 0.00;
  doc["readings"]["humidity"] = 0.00;

  // TEMP_HACK
  event.temperature = 25.5;
  event.relative_humidity = 65.0;
  snprintf(buff+strlen(buff), SERIAL_PRINTF_MAX_BUFF - strlen(buff), "[%04d]:", count);
  snprintf(buff+strlen(buff), SERIAL_PRINTF_MAX_BUFF - strlen(buff), " Temp: %0.2f°C,", event.temperature);
  doc["readings"]["temp"] = event.temperature;
  snprintf(buff+strlen(buff), SERIAL_PRINTF_MAX_BUFF - strlen(buff), " \tHumidity: %0.2f%\0", event.relative_humidity);
  doc["readings"]["humidity"] = event.relative_humidity;

  serializeJson(doc, buff+strlen(buff), SERIAL_PRINTF_MAX_BUFF - strlen(buff));
  // This prints:
  // {"sensor":"gps","time":1351824120,"data":[48.756080,2.302038]}
  Serial.println(buff);
}

/*---------------------------------------------------------------------------*/
void serialPrintf(const char *fmt, ...) {
  /* Buffer for storing the formatted data */
  buff[0] = 0;
  /* pointer to the variable arguments list */
  va_list pargs;
  /* Initialise pargs to point to the first optional argument */
  va_start(pargs, fmt);
  /* create the formatted data and store in buff */
  vsnprintf(buff, SERIAL_PRINTF_MAX_BUFF, fmt, pargs);
  va_end(pargs);
  Serial.print(buff);
}

/*---------------------------------------------------------------------------*/
int handle_serial_event(String event_str) {
  static const String inc_delay_str = String("INC_DELAY");
  static const String dec_delay_str = String("DEC_DELAY");
  static const String resume_str = String("RESUME");
  static const String pause_str = String("PAUSE");
  static const String stop_str = String("STOP");
  event_str.toUpperCase();
  if (event_str == stop_str) {
    serialPrintf(">>> Executing Command: [%s]\n", event_str.c_str());
    exit_loop = true;
  } else if (event_str == pause_str) {
    skip_loop = true;
    serialPrintf(">>> Executing Command: [%s]\n", event_str.c_str());
  } else if (event_str.equals(resume_str)) {
    skip_loop = false;
    serialPrintf(">>> Executing Command: [%s]\n", event_str.c_str());
  } else if (event_str == inc_delay_str) {
    delayMS_loop += 10000;
    serialPrintf(">>> Executing Command: [%s]\n", event_str.c_str());
  } else if (event_str == dec_delay_str) {
    delayMS_loop -= 10000;
    if (delayMS_loop < 1000) {
      delayMS_loop = 1000;
    }
    serialPrintf(">>> Executing Command: [%s]\n", event_str.c_str());
  } else {
    serialPrintf("#### Unknown Command: [%s]\n", event_str.c_str());
  }
}


