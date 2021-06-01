#include <WiFi.h>
#include <Arduino.h>
#include <AsyncDelay.h>
#include <AsyncMqttClient.h>

#define SSID "SSID"
#define PASSWORD "PASSWORD"

#define DOOR_1_PIN 18
#define DOOR_2_PIN 19

#define MQTT_HOST IPAddress(192, 168, 50, 250)
#define MQTT_PORT 1883


TaskHandle_t task1;
AsyncDelay delayDoorOneTimer;
AsyncDelay delayDoorTwoTimer;
AsyncMqttClient mqttClient;


void check_timer_expired_task(void* pvParameters);
void wifi_init();
void mqtt_init();
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);


void setup(){
  Serial.begin(115200);
  pinMode(DOOR_1_PIN,OUTPUT);
  pinMode(DOOR_2_PIN,OUTPUT);

  wifi_init();
  mqtt_init();
  
  xTaskCreatePinnedToCore(
    check_timer_expired_task,
    "checkNode",
    40000,
    NULL,
    -1,
    &task1,
    tskNO_AFFINITY);
}



void check_timer_expired_task(void* pvParameters){
  while(true){
    if (delayDoorOneTimer.isExpired() && digitalRead(DOOR_1_PIN)){
      digitalWrite(DOOR_1_PIN,LOW);
      Serial.println("Door_1 locked.");
    }
    
    if (delayDoorTwoTimer.isExpired() && digitalRead(DOOR_2_PIN)){
      digitalWrite(DOOR_2_PIN,LOW);
      Serial.println("Door_2 locked.");
    }
    
    delay(100);
  }
}


void wifi_init(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID,PASSWORD);
  
  Serial.print("Connect to WiFi");
  while (!WiFi.isConnected()){
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  Serial.print("Connected\nWifi IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Mac address:");
  Serial.println(WiFi.macAddress());
}


void mqtt_init(){
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(3);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.connect();
  
  while (!mqttClient.connected()) delay(100);
  mqttClient.subscribe("door_1",2);
  mqttClient.subscribe("door_2",2);
  Serial.println("MQTT Connected.");
}


void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
    if (strncmp(topic,"door_1",6) == 0){
      if ( len==2 && strncmp(payload,"On",2) == 0){
        if (delayDoorOneTimer.isExpired()){
          Serial.println("Open door_1");
          digitalWrite(DOOR_1_PIN,HIGH);
          delayDoorOneTimer.start(6000,AsyncDelay::MILLIS);
        }else{
          Serial.println("Renew the timer of door_1");
          delayDoorOneTimer.expire();
          delayDoorOneTimer.start(6000,AsyncDelay::MILLIS);
        }
      
      }
    }else if (strncmp(topic,"door_2",6) == 0){
      if ( len==2 && strncmp(payload,"On",2) == 0){
        if (delayDoorTwoTimer.isExpired()){
          Serial.println("Open door_2");
          digitalWrite(DOOR_2_PIN,HIGH);
          delayDoorTwoTimer.start(6000,AsyncDelay::MILLIS);
        }else{
          Serial.println("Renew the timer of door_2.");
          delayDoorTwoTimer.expire();
          delayDoorTwoTimer.start(6000,AsyncDelay::MILLIS);
        }
      }
    }
}


void loop(){
}
