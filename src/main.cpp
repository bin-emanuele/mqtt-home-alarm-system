/*
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "RTClib.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "PCF8574.h"

#include <SPI.h> // not used here, but needed to prevent a RTClib compile error

#include "MotionSensor.hpp"

// Update these with values suitable for your network.

#define MSG_BUFFER_SIZE (50)
#define MAX_SENSORS 12

const char *SSID = "SmartHome";
const char *WIFI_PASSWORD = "MicaTanto";
const char *MQTT_SERVER = "192.168.1.31";
const char *MQTT_TOPIC_OUT = "mhas";
const char *MQTT_TOPIC_IN = "mhas/commands";

WiFiClient espClient;
PubSubClient client(espClient);

RTC_DS3231 rtc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

MotionSensor sensors[MAX_SENSORS];
PCF8574 sensorsBlock1(0x20);
PCF8574 sensorsBlock2(0x21);
PCF8574 sensorsBlock3(0x22);

char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.begin(115200);
    Serial.print("Connecting to ");
    Serial.println(SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup_time()
{
    // Retrieve time from NTP
    Serial.println("");
    Serial.println("Retrieving time from NTP: ");

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(10);
    }

    timeClient.begin();
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
    rtc.adjust(DateTime(timeClient.getEpochTime()));
}

void setup_sensors()
{
    Serial.println("Setting up sensors...");

    sensors[0] = MotionSensor("mhas/pir/interno/soggiorno-1", &sensorsBlock1, P0, P1);
    sensors[1] = MotionSensor("mhas/pir/interno/soggiorno-2", &sensorsBlock1, P2, P3);
    sensors[2] = MotionSensor("mhas/pir/interno/corridoio", &sensorsBlock1, P4, P5);
    sensors[3] = MotionSensor("mhas/pir/interno/matrimoniale", &sensorsBlock1, P6, P7);

    Serial.print("Init sensorsBlock1... ");
    if (sensorsBlock1.begin())
    {
        Serial.println("OK");
    }
    else
    {
        Serial.println("KO");
    }

    sensors[4] = MotionSensor("mhas/pir/interno/garage", &sensorsBlock2, P0, P1);
    // sensors[5] = MotionSensor("mhas/pir/interno/xxx", &sensorsBlock2, P2, P3);
    // sensors[6] = MotionSensor("mhas/pir/interno/yyy", &sensorsBlock2, P4, P5);
    // sensors[7] = MotionSensor("mhas/pir/interno/zzz", &sensorsBlock2, P6, P7);

    Serial.print("Init sensorsBlock2... ");
    if (sensorsBlock2.begin())
    {
        Serial.println("OK");
    }
    else
    {
        Serial.println("KO");
    }

    sensors[8] = MotionSensor("mhas/pir/esterno/fronte", &sensorsBlock3, P0, P1);
    sensors[9] = MotionSensor("mhas/pir/esterno/retro", &sensorsBlock3, P2, P3);
    sensors[10] = MotionSensor("mhas/pir/esterno/portico", &sensorsBlock3, P4, P5);
    sensors[11] = MotionSensor("mhas/pir/esterno/veranda", &sensorsBlock3, P6, P7);

    Serial.print("Init sensorsBlock3... ");
    if (sensorsBlock3.begin())
    {
        Serial.println("OK");
    }
    else
    {
        Serial.println("KO");
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }

    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1')
    {
        digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
                                        // but actually the LED is on; this is because
                                        // it is active low on the ESP-01)
    }
    else
    {
        digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.println("Attempting MQTT connection...");

        // Create a random client ID
        String clientId = "mqtt-home-allarm-system-";
        clientId += String(random(0xffff), HEX);

        // Attempt to connect
        if (client.connect(clientId.c_str(), "mqtt", "NelDubbioMQTT"))
        {
            Serial.println("Connected...");
            // Subscribe
            client.subscribe(MQTT_TOPIC_IN);
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    Serial.begin(115200);

    setup_wifi();
    setup_time();
    setup_sensors();

    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(callback);
}

void loop()
{
    // Check mqtt connection
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Check all the motion sensors
    for (size_t i = 0; i < MAX_SENSORS; i++)
    {
        // Check if initialized
        if (!sensors[0].isInitialized()) {
            continue;
        }

        // Read values
        sensors[0].readValues();

        // Values have hanged?
        if (!sensors[0].isChanged()) {
            continue;
        }

        String s = sensors[0].generatePayloadJSON(rtc.now());

        Serial.print("Publish message: ");
        Serial.print(sensors[0].getTopic());
        Serial.println(": ");
        Serial.print(s);
        Serial.println("");

        client.publish(sensors[0].getTopic(), s.c_str(), false);
    }

    delay(250);
}