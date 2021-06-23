#include <WiFi.h>
#include <Arduino.h>
#include <AsyncDelay.h>
#include <AsyncMqttClient.h>

#define SSID "SSID"
#define PASSWORD "YOUR_PASSWORD"

#define ENTRANCE_PIN_1 16
#define ENTRANCE_PIN_2 17
#define EXIT_PIN_1 18
#define EXIT_PIN_2 19

#define ENTRANCE_STOP_TRIGGER_OPEN 32
#define ENTRANCE_STOP_TRIGGER_CLOSE 33
#define EXIT_STOP_TRIGGER_OPEN 12
#define EXIT_STOP_TRIGGER_CLOSE 14



#define MQTT_HOST IPAddress(192, 168, 50, 250)
#define MQTT_PORT 1883


TaskHandle_t task1;

AsyncDelay delayDoorInTimer;
AsyncDelay delayDoorOutTimer;
AsyncMqttClient mqttClient;

bool doorInLastMove = false; // true -> open || false -> close
bool doorOutLastMove = false;

void check_timer_expired_task(void* pvParameters);
void wifi_init();
void mqtt_init();
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);


void setup(){
  Serial.begin(115200);
  pinMode(EXIT_PIN_1,OUTPUT);
  pinMode(EXIT_PIN_2,OUTPUT);
  pinMode(ENTRANCE_PIN_1,OUTPUT);
  pinMode(ENTRANCE_PIN_2,OUTPUT);

  pinMode(ENTRANCE_STOP_TRIGGER_OPEN,INPUT_PULLUP);
  pinMode(ENTRANCE_STOP_TRIGGER_CLOSE,INPUT_PULLUP);
  pinMode(EXIT_STOP_TRIGGER_OPEN,INPUT_PULLUP);
  pinMode(EXIT_STOP_TRIGGER_CLOSE,INPUT_PULLUP);
  
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
    if (delayDoorInTimer.isExpired() && doorInLastMove ){
      digitalWrite(ENTRANCE_PIN_1,LOW);
      digitalWrite(ENTRANCE_PIN_2,HIGH);
      doorInLastMove = false;
      Serial.println("Door_1 Close.");
    }

    if (delayDoorOutTimer.isExpired() && doorOutLastMove ){
      digitalWrite(EXIT_PIN_1,LOW);
      digitalWrite(EXIT_PIN_2,HIGH);
      doorOutLastMove = false;
      Serial.println("Door_2 Close.");
    }
    
    // When stop flag triggered
    if (  (digitalRead(ENTRANCE_STOP_TRIGGER_OPEN) == LOW && doorInLastMove)
        ||(digitalRead(ENTRANCE_STOP_TRIGGER_CLOSE) == LOW && !doorInLastMove))
    {
      digitalWrite(ENTRANCE_PIN_1,LOW);
      digitalWrite(ENTRANCE_PIN_2,LOW);
    }

    if (  (digitalRead(EXIT_STOP_TRIGGER_OPEN) == LOW && doorOutLastMove)
        ||(digitalRead(EXIT_STOP_TRIGGER_CLOSE) == LOW && !doorOutLastMove))
    {
      digitalWrite(EXIT_PIN_1,LOW);
      digitalWrite(EXIT_PIN_2,LOW);
    }
    
    delay(10);
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
  mqttClient.subscribe("door_1",0);
  mqttClient.subscribe("door_2",0);
  Serial.println("MQTT Connected.");
}


void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){
    if (strncmp(topic,"door_1",6) == 0){
      if ( len==2 && strncmp(payload,"On",2) == 0){
        if (delayDoorInTimer.isExpired()){
          Serial.println("Open door_1");
          delayDoorInTimer.start(6000,AsyncDelay::MILLIS);
        }else{
          Serial.println("Renew the timer of door_1");
          delayDoorInTimer.expire();
          delayDoorInTimer.start(6000,AsyncDelay::MILLIS);
        }
        digitalWrite(ENTRANCE_PIN_1,HIGH);
        digitalWrite(ENTRANCE_PIN_2,LOW);
        doorInLastMove = true;
      }
    }else if (strncmp(topic,"door_2",6) == 0){
      if ( len==2 && strncmp(payload,"On",2) == 0){
        if (delayDoorOutTimer.isExpired()){
          Serial.println("Open door_2");
          delayDoorOutTimer.start(6000,AsyncDelay::MILLIS);
        }else{
          Serial.println("Renew the timer of door_2");
          delayDoorOutTimer.expire();
          delayDoorOutTimer.start(6000,AsyncDelay::MILLIS);
        }
        digitalWrite(EXIT_PIN_1,HIGH);
        digitalWrite(EXIT_PIN_2,LOW);
        doorOutLastMove = true;
      }
    }
}


void loop(){
}
