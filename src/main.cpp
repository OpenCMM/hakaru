#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ConfigManager.h>
#include <Sensor.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

void ledBlink();
void callback(char* topic, byte* message, unsigned int length);
void reconnect();
void publishSensorData(int currentData, std::pair<time_t, int>);

const int touchPin = 4; // GPIO4 as the touch-sensitive pin
int sensorData = 0;
int interval = 1000; // 1 second
int threshold = 1000;
const char* hostname = "opencmm";
const char* mqttServer = "192.168.10.116";
const int mqttPort = 1883;
const char* mqttUser = "opencmm";
const char* mqttPassword = "opencmm";

const char* clientId = "ESP32Client";
const char* sensorTopic = "sensor/data";
const char* controlTopic = "sensor/control";
const char* pingTopic = "sensor/ping";
const char* pongTopic = "sensor/pong";

int lastMillis = 0;
int intervalCounter = 0;
int totalInterval = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void touchCallback()
{
  // wake up from deep sleep
  Serial.println("Touch detected");
  // turn on sensor
  switchSensor(true);
  ledBlink();
}

void setup()
{
  Serial.begin(115200);
  checkWifiInfo();
  MDNS.begin(hostname);
  setupSensor();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  touchAttachInterrupt(touchPin, touchCallback, 40); // Attach touch interrupt

  // Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  ledBlink();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  // check payload is 'ping' and send response 'pong'
  if (strcmp(topic, pingTopic) == 0 && strncmp((char*)payload, "ping", 4) == 0) {
    client.publish(pongTopic, "pong");
  } else if (strcmp(topic, controlTopic) == 0) {
    char receivedJson[200];
    for (int i = 0; i < length; i++) {
      receivedJson[i] = (char)payload[i];
    }
    receivedJson[length] = '\0';
    // Deserialize the received JSON string
    const uint8_t size = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<size> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, receivedJson);

    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
    } else {
      const char *command = jsonDoc["command"];
      if (strcmp(command, "config") == 0) {
        const int _interval = jsonDoc["interval"];
        if (isIntervalValid(_interval))
        {
          interval = _interval;
          Serial.println("Interval: " + String(interval));
        }

        const int _threshold = jsonDoc["threshold"];
        if (isThresholdValid(_threshold))
        {
          threshold = _threshold;
          Serial.println("Threshold: " + String(threshold));
        }
        Serial.println("Configured");
        Serial.print("Received JSON - Threshold: ");
        Serial.print(threshold);
        Serial.print(", Interval: ");
        Serial.print(interval);
        Serial.print(", Command: ");
        Serial.println(command);
      } else if (strcmp(command, "deepSleep") == 0)
      {
        Serial.println("Going to sleep now");
        delay(1000);
        // turn off sensor
        switchSensor(false);
        delay(1000);
        esp_deep_sleep_start();
      } else if (strcmp(command, "resetWifi") == 0) {
        Serial.println("Resetting Wi-Fi credentials");
        resetWifiCredentialsWithWs();
      }
    }
  }
}

void publishSensorData(int currentData, std::pair<time_t, int> currentTime) {
  const uint8_t size = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<size> doc;
  doc["data"] = currentData;
  doc["timestamp"] = currentTime.first;
  doc["millis"] = currentTime.second;
  char message[200];
  serializeJson(doc, message);
  client.publish(sensorTopic, message);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    std::pair<int16_t, std::pair<time_t, int>> sensorDataPair = getSensorData();
    std::pair<time_t, int> currentTime = sensorDataPair.second;
    // calculate loop interval
    int loopInterval = 0;
    if (currentTime.second > lastMillis) {
      loopInterval = currentTime.second-lastMillis;
    } else {
      loopInterval = 1000 - lastMillis + currentTime.second;
    }
    totalInterval += loopInterval;
    intervalCounter++;
    if (intervalCounter == 1000) {
      Serial.println("Average loop interval: " + String(totalInterval / intervalCounter));
      intervalCounter = 0;
      totalInterval = 0;
    }
    lastMillis = currentTime.second;

    if (interval != 0) {
      delay(interval);
    }
  } else {
    runServer();
    delay(1000);
    if (!checkIfScanCompleted()) {
      scanNetworks();
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(controlTopic);
      client.subscribe(pingTopic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

// LED blink
void ledBlink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}
