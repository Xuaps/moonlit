#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char *AP_SSID = "";
const char *AP_PASS = "";
#define IOTHUB_HOSTNAME   ""
#define DEVICE_ID         ""
#define DEVICE_KEY        ""
#define DHTPIN            2
#define DHTTYPE           DHT11

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);
WiFiEventHandler  e1, e2;
DHT dht(DHTPIN, DHTTYPE);
unsigned long last_time_on = 0;
unsigned long on_time = 14400;
unsigned long time_between_ignitions = 28800; 
int limit_value = 950;

void connectToIoTHub(); // <- predefine connectToIoTHub() for setup()

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
    Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());

    //do connect upon WiFi connected
    connectToIoTHub();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
    Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
    Serial.printf("Reason: %d\n", event_info.reason);
}

void connectToIoTHub() {
    Serial.print("\nBeginning Azure IoT Hub Client... ");
    if (client.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("Could not connect to MQTT");
    }
}

void onClientEvent(const AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event) {
    if (event == AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEventConnected) {
        Serial.println("Connected to Azure IoT Hub");
    }
}

int luminosity() {
    return analogRead(A0); 
}

int temperature() {
    return dht.readTemperature();
}

int humidity() {
    return dht.readHumidity();
}

unsigned long delta(unsigned long time1, unsigned long time2) {
    return time1 - time2;
}

void notify (int relay_status) {
    AzureIoTHubMQTTClient::KeyValueMap keyVal = {
        {"DeviceId", DEVICE_ID},
        {"Luminosity", luminosity()},
        {"temperature", temperature()},
        {"humidity", humidity()},
        {"relay_status", relay_status},
        {"last_time_on", last_time_on},
        {"current_timestamp", now()}
    };
    client.sendEventWithKeyVal(keyVal);
}

void setup() {      //Configuracion de los pin como entrada o salida 
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  while(!Serial) {
    yield();
  }
  delay(2000);


  pinMode(D1,OUTPUT);
  pinMode(A0,INPUT);

  dht.begin();
  delay(20);

  Serial.print("Connecting to WiFi...");
  //Begin WiFi joining with provided Access Point name and password
  WiFi.begin(AP_SSID, AP_PASS);

  //Handle WiFi events
  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start the Client
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

  //Handle client events
  client.onEvent(onClientEvent);
}

void loop ()  {
    client.run();
    if (luminosity() > limit_value && delta(now(), last_time_on)>time_between_ignitions) {
        digitalWrite(D1, HIGH);
        last_time_on = now();
    }else if (luminosity() <= limit_value || delta(now(), last_time_on)>on_time){
        digitalWrite(D1, LOW);
    }

    if (client.connected())
    {
        notify(digitalRead(D1));
    }
    delay(500);
}
