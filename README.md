# esp8266_01_mqtt_to_relay

Using the ESP-01 module to control a Relay using MQTT

---

## Arduino IDE Config:

### Required Libaries

    - ESP8266 Boards Manager: http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/installing.html
    - AdaFruit_MQTT: https://learn.adafruit.com/mqtt-adafruit-io-and-you/arduino-plus-library-setup
    - ArduinoJson: https://arduinojson.org/

### Board Config

    - Boards Manager: "esp8266 by ESP8266 Community" (ref: https://arduino-esp8266.readthedocs.io/en/latest/)
    - Board Type: Generic ESP8266 (Tools->Board->Generic ESP8266 Module)
    - Baud Rate: 115200


---

## Details required:

Fill the following details for connecting to the WiFi router (at your home):
    - WLAN_SSID
    - WLAN_PASS

As this is a Demo project, I have used AdaFruit IO's Feeds as the MQTT Broker to communicate with the Device.
(see https://accounts.adafruit.com).
You will need the following details of your AdaFruit IO (AIO) account:
    - AIO_USERNAME
    - AIO_KEY                   <= This can be accessed/generated from: https://io.adafruit.com/<AIO_USERNAME>/dashboards (look for the "My Key" tab on the menubar).
    - AIO_MQTT_FEED_ONOFFBUTTON <= The name of the feed to which the Device should subscribe (would start with something like "/feeds/<FEED_NAME>").

---

## References

1. [Using ESP8266 GPIO0/GPIO2/GPIO15 pins](http://www.forward.com.au/pfod/ESP8266/GPIOpins/index.html)
2. [How to Use the ESP8266-01 Pins and Leds](https://www.instructables.com/How-to-use-the-ESP8266-01-pins/)
3. [ESP8266 Pinout Reference](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
4. [Beginners Guide To The Paho MQTT Python Client](http://www.steves-internet-guide.com/into-mqtt-python-client/)
5. http://test.mosquitto.org/