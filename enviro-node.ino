// Code based on
// https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHTtester/DHTtester.ino
// https://gist.github.com/igrr/7f7e7973366fc01d6393
// https://github.com/iot-playground/Arduino/blob/master/ESP8266ArduinoIDE/DS18B20_temperature_sensor/DS18B20_temperature_sensor.ino

// esp8266 + dht22 + mqtt

#include "DHT.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* ssid = "<ssid>";
const char* password = "<wifi pass>";
const char* mqtt_user = "<user>";
const char* mqtt_pass = "<pass>";

bool debug = true;

String topic = "hass/enviro/";
char* server = "<mqtt server>";
String hellotopic = "hass/enviro/";

#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define REPORT_INTERVAL 30 // in sec


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

String clientName;
DHT dht(DHTPIN, DHTTYPE, 15);
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

float oldH ;
float oldT ;

void setup() {
  if (debug){
    Serial.begin(115200);
    Serial.println("DHTxx test!");
  }
  delay(20);

  if (debug) {
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (debug) {Serial.print(".");}
  }
  if (debug){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  


  uint8_t mac[6];
  WiFi.macAddress(mac);

  topic += macToStr(mac);
  hellotopic += macToStr(mac);
  hellotopic += "/status";
  clientName += "enviro-";
  
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  if (debug){
    Serial.print("Connecting to ");
    Serial.print(server);
    Serial.print(" as ");
    Serial.print(clientName);
    Serial.print(" and using user ");
    Serial.println(mqtt_user);
  }
  if (client.connect((char*) clientName.c_str(),mqtt_user,mqtt_pass)) {
    if (debug){ 
      Serial.println("Connected to MQTT broker");
      Serial.print("Topic is: ");
      Serial.println(topic);
    }
    String joinMsg = "{\"deviceid\":";
    joinMsg += clientName;
    joinMsg += "}";
    
    if (client.publish((char*) hellotopic.c_str(), (char*) joinMsg.c_str())) {
      if (debug){Serial.println("Publish ok");}
    }
    else {
      if (debug){Serial.println("Publish failed");}
    }
  }
  else {
    if (debug){
      Serial.println("MQTT connect failed");
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(1000);
      Serial.println("Will reset and try again...");
    }
    ESP.restart();
  }

  dht.begin();
  oldH = -1;
  oldT = -1;
}

void loop() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    if (debug){Serial.println("Failed to read from DHT sensor!");}
    delay(20);
    return;
  }

  float hi = dht.computeHeatIndex(f, h);
  if (debug){
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(hi);
    Serial.println(" *F");
  }
  String payload = "{\"humidity\" : ";
  payload += h;
  payload += ", \"h_units\" : \"%\", \"temperature\":";
  payload += f;
  payload += ", \"t_units\" : \"f\"}";
 
 
  if (t != oldT || h != oldH )
  {
    sendTemperature(payload);
    oldT = t;
    oldH = h;
  }

  int cnt = REPORT_INTERVAL;

  while (cnt--){
    client.loop();
    delay(1000);
  }
}


void sendTemperature(String payload) {
  if (!client.connected()) {
    if (client.connect((char*) clientName.c_str())) {
      if (debug){
        Serial.println("Connected to MQTT broker again");
        Serial.print("Topic is: ");
        Serial.println(topic);
      }
    }
    else {
      if (debug){
        Serial.println("MQTT connect failed");
        Serial.println("Will reset and try again...");
      }
      ESP.restart();
    }
  }

  if (client.connected()) {
    if (debug){
      Serial.print("Sending payload: ");
      Serial.println(payload);
    }

    if (client.publish((char*) topic.c_str(), (char*) payload.c_str())) {
      if (debug){Serial.println("Publish ok");}
    }
    else {
      if (debug){Serial.println("Publish failed");}
    }
  }

}



String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
