#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "ThingSpeak.h"
#include <ESP8266WiFi.h>

const char[] APIkey = "";
const int channelNumber = 4;

static const int RXPin = 4, TXPin = 5;
static const uint32_t GPSBaud = 9600;

while (ss.available() > 0)
{
    if (gps.encode(ss.read()))
    {
        if (gps.location.isValid())
        {
            latitude = gps.location.lat();
            lat_str = String(latitude, 6);
            longitude = gps.location.lng();
            lng_str = String(longitude, 6);
            Serial.print("Latitude = ");
            Serial.println(lat_str);
            Serial.print("Longitude = ");
            Serial.println(lng_str);

            ThingSpeak.setField(1, lat_str);
            ThingSpeak.setField(2, lng_str);
            ThingSpeak.writeFields(ChannelNumber, APIkey);
            delay(1000);
            Serial.println();
        }
    }
}
