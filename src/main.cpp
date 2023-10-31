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
void publishSensorData(int sensorData);

const int touchPin = 4; // GPIO4 as the touch-sensitive pin
int sensorData = 0;
int interval = 1000; // 1 second
int threshold = 100;
const char* hostname = "opencmm";
const char* mqttServer = "192.168.10.104";
const int mqttPort = 1883;
const char* mqttUser = "opencmm";
const char* mqttPassword = "opencmm";

const char* clientId = "ESP32Client";
const char* sensorTopic = "sensor/data";
const char* controlTopic = "sensor/control";


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

void publishSensorData(int sensorData) {
  char message[20];
  snprintf(message, 20, "%d", sensorData);
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

    int currentData = getSensorData();
    // if the difference is greater than threshold, send data
    if (abs(currentData - sensorData) < threshold) {
      return;
    }
    sensorData = currentData;
    Serial.println(currentData);
    publishSensorData(currentData);

    delay(interval);
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
