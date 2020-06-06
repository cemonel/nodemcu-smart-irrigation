#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include "DHT.h"
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <Arduino_JSON.h>

#define DHTTYPE DHT11

const char *ssid = "TTNET_TPLINK_E9B0";
const char *password = "azbwuu4d";
uint8_t DHTPin = D7;
uint8_t motor_pin1 = D1;
uint8_t motor_pin2 = D2;
uint8_t motor_pin3 = D3;

const int analogInPin = 0;  // Analog input pin that the potentiometer is attached to
int sensorValue = 0;        // value read from the potentiometer
int outputValue = 0;        // value sent to server

DHT dht(DHTPin, DHTTYPE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

float temperature;
float humidty;

int soil_sensor_pin = 0;

String sensor_values;

WiFiClient client;
String url;
String irrigation_url;
String response;
const char * host = "192.168.1.100";
const int httpPort = 7373;
unsigned long timeout = millis();
int count = 0;

void connect_to_wifi(){

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      }
    
  Serial.println("");
  Serial.println("WiFi connected."); 
  Serial.println("cemcem."); 
  
}

void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(motor_pin1, OUTPUT);
  pinMode(motor_pin2, OUTPUT);
  pinMode(motor_pin3, OUTPUT);
  digitalWrite(motor_pin1, LOW);
  digitalWrite(motor_pin2, LOW);
  digitalWrite(motor_pin3, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  // Explicitly set the ESP8266 to be a WiFi-client
  connect_to_wifi();
  delay(100);
  timeClient.begin();
  pinMode(DHTPin, INPUT);
  dht.begin();
}

void loop() {
  // read the analog in value:
  //sensorValue = analogRead(A0);
  // map to range. The pot goes from about 3 to 1023. This makes the sent value be between 0 and 999 to fit on the OLED
  //outputValue = map(sensorValue, 3, 1023, 0, 999);

  // Use WiFiClient class to create TCP connections
  digitalWrite(LED_BUILTIN, HIGH);

  delay(1000);

  HTTPClient http;
  
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  timeClient.update();
  sensor_values = String(timeClient.getEpochTime()) + String("&soil_moisture=") + String(analogRead(soil_sensor_pin)) + String("&air_temperature=") + String(dht.readTemperature()) + String("&air_humidity=") + String(dht.readHumidity());
  // We now create a URI for the request. Something like /data/?sensor_reading=123
  url = "/data/";
  irrigation_url = "/plant/1/irrigation/";
  url += "?time=";
  url += sensor_values;

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

    timeout = millis();
      
    while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  count = 0;
  while(count < 28){
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }

    String response;
    String serverPath = "http://192.168.1.100:7373/plant/1/irrigation/";
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0){
          response = http.getString();  
    }

    http.end();
    JSONVar status_object = JSON.parse(response);
    String jsonString = JSON.stringify(status_object["status"]);
    Serial.print(jsonString);
    if (jsonString.equals("\"Irrigate\"")){ // irrigation time
      digitalWrite(motor_pin1, HIGH);
      digitalWrite(motor_pin2, HIGH);
      Serial.print("Irr start");
      delay(300000);
      Serial.print("Irr end");
      Serial.print(jsonString);
      digitalWrite(motor_pin1, LOW);
      digitalWrite(motor_pin2, LOW);
     }
    count++;
    delay(2000);
  }  
  
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
