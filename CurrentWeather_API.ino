#include <ArduinoJson.h>

#include <Arduino_JSON.h>

#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiType.h>
#include <WiFiUdp.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "ESP32"
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define INFLUXDB_URL "https://eu-central-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN ""
#define INFLUXDB_ORG "@e-uvt.ro"
#define INFLUXDB_BUCKET "currentweather"
#define TZ_INFO "CET-2CEST,M3.5.0,M10.5.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

float main_temp = 0;
int main_humidity = 0;
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?q=Sag,ro&APPID=";
const String key = "";
//const String endpoint1 = "http://api.openweathermap.org/data/2.5/forecast?id=668269&appid=";

Point sensor("wifi_status");
void setup() {

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi network: ");
    Serial.println(WIFI_SSID);
  }

  Serial.println("Connected to the WiFi network");
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}



void loop() {
  // getting data from the API

  HTTPClient http;
  http.begin(endpoint + key); //Specify the URL
  int httpCode = http.GET();  //Make the request
  if (httpCode > 0) { //Check for the returning code

    String payload = http.getString();

    // Filter the payload (json received data)

    StaticJsonDocument<112> filter;
    filter["weather"][0]["main"] = true;
    JsonObject filter_main = filter.createNestedObject("main");
    filter_main["temp"] = true;
    filter_main["humidity"] = true;
    filter["name"] = true;

    StaticJsonDocument<192> doc;
    DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    const char* weather_0_main = doc["weather"][0]["main"]; // "Clear"
    float main_temp = doc["main"]["temp"]; // 22.89
    int main_humidity = doc["main"]["humidity"]; // 42
    const char* name = doc["name"];

    //sending data to DB
    sensor.clearFields();
    sensor.addField("temperature", main_temp);
    sensor.addField("humidity", main_humidity);
    Serial.print("Writing: ");
    Serial.println(sensor.toLineProtocol());

    if (!client.writePoint(sensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }

    //print values
    Serial.print("Current weather in ");
    Serial.println(name);
    //Serial.println(httpCode);
    Serial.print(main_temp);
    Serial.print("°C ");
    Serial.print(main_humidity);
    Serial.println("%");
  }
  else {
    Serial.println("Error on HTTP request");
  }
  http.end(); //Free the resources

  Serial.println("Writing done.");
  delay(30000);
}
