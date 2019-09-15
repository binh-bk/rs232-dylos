/*
Binh Nguyen, July 13, 2019:
- listen to RS-232 data
- push data to MQTT server
*/

/*_____________________BASED LIBRARIES_______________________*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

WiFiClient espClient;
PubSubClient client(espClient);

/*_____________________WIFI and MQTT_______________________*/
#define wifi_ssid "your SSID"
#define wifi_password "your wifi password"
#define mqtt_server "192.168.1.xx" // MQTT IP
#define mqtt_user "mqtt_user" 
#define mqtt_password "mqtt_password"
#define mqtt_port 1883
#define publish_topic "air/dylos"

/*_____________________OverAIR (OTA)_______________________*/
#define SENSORNAME "dc1100"
#define OTApassword "ota_password"  //use for update software over air
int OTAport = 8266;

//SoftwareSerial swSer(RX, TX, false, buffer_size);
SoftwareSerial swSer(D6, D5, false, 256);
uint32_t timestamp = 0;
String content = "";
char character;
String small, large;

/*_____________________START SETUP_______________________*/
void setup() {
  Serial.begin(9600);
  swSer.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT), 
  setup_wifi();
  flashLED(500,3);
  setup_OTA();

  client.setServer(mqtt_server, mqtt_port);
}

/*_____________________MAIN LOOP_______________________*/
void loop() {
  ArduinoOTA.handle();
  client.loop();
  
  while (swSer.available() > 0) {
    character = swSer.read();
    content.concat(character);
  }
  if (content.length() > 3){
    Serial.println(content);
//    split content and strim white space,
    small = getValue(content, ',', 0);
    small.trim();
    large = getValue(content, ',', 1);
    large.trim();
    Serial.println(small);
    Serial.println(large);
    timestamp = millis()/1000;
    pushData();
    content = "";
    swSer.flush();
    flashLED(200,5);
  }
  delay(100);
}

/*___________________SPLIT STR BY COMMA_____________________*/
// written by someone else

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
/*_____________________START OT_______________________*/
//standard OTA function
void setup_OTA(){
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPassword((const char *)OTApassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

/*______________        _ SETUP WIFI _        _______________*/
void setup_wifi() {
  delay(10);
  Serial.printf("Connecting to %s", wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  delay(100); 
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    i++;
    Serial.printf(" %i ", i);
    if (i == 5){
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifi_ssid, wifi_password);
      delay(1000);
    }
    if (i >=10){
      ESP.restart();
      Serial.println("Resetting ESP");
    }
  }
  Serial.printf("\nWiFi connected: \t");
  Serial.print(WiFi.localIP());
  Serial.print("\twith MAC:\t");
  Serial.println(WiFi.macAddress());
}

/*_____________________SEND STATE_______________________*/
//customize your key if you wish so
void pushData() {
  StaticJsonDocument<512> doc;
  
  doc["sensor"] = SENSORNAME;
  doc["uptime"] = timestamp;
  doc["small"] = small;
  doc["large"] = large;
  
  size_t len = measureJson(doc)+ 1;
  char payload[len];
  serializeJson(doc, payload, sizeof(payload));
    if (!client.connected()) {
    reconnect();
    delay(1000);
  }
  if (client.publish(publish_topic, payload, false)){
    Serial.println("Success: " + String(payload));
  } else {
    Serial.println("Failed to push: " + String(payload));
  }
}
/*_____________________START RECONNECT_______________________*/
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
//      pushData();
//    flashLED(50,3);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

/*_____________________ FLASH LED______________________*/
//make some visual signal when deploy the board

void flashLED(int onTime, int cycle){
  for (int i=0;i<cycle; i++){
    digitalWrite(LED_BUILTIN, 0);
    delay(onTime);
    digitalWrite(LED_BUILTIN, 1);
    delay(onTime);
  }
}
