#!/bin/bash

apt list mosquitto | grep mosquitto
if [ $? -ne 0 ] ; then
	echo "MQTT Client not installed!!!"
	echo "try running `sudo apt install mosquitto-clients`"
	exit -1
fi

AIO_SERVER="io.adafruit.com"
AIO_USERNAME="__UPDATE_ME__"                    # ...your AIO username (see https://accounts.adafruit.com)...
AIO_KEY="__UPDATE_ME__"                         # ...your AIO key...
AIO_MQTT_FEED_ONOFFBUTTON="__UPDATE_ME__"       # ...your AIO feed to use for communicating over MQTT...

ON_MESSAGE="{\"dest\":\"sw_01\",\"cmd\":\"turn_on\"}"
OFF_MESSAGE="{\"dest\":\"sw_01\",\"cmd\":\"turn_off\"}"

echo "Switching ON the Relay..."
mosquitto_pub -h ${AIO_SERVER} -t "${AIO_USERNAME}/feeds/${AIO_MQTTFEED_ONOFFBUTTON}" -u ${AIO_USERNAME} -P "${AIO_KEY}" -m "${ON_MESSAGE}"
sleep 10
echo "Switching OFF the Relay..."
mosquitto_pub -h ${AIO_SERVER} -t "${AIO_USERNAME}/feeds/${AIO_MQTTFEED_ONOFFBUTTON}" -u ${AIO_USERNAME} -P "${AIO_KEY}" -m "${OFF_MESSAGE}"
