#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"

// Wi-Fi and MQTT credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;

// MQTT topics
const char* mqtt_topic_pixels = "student/CASA0014/light/1/pixel/";

// Sensor pins
#define LDR_PIN A0        // 光敏电阻引脚
#define TRIG_PIN 6        // 超声波Trig引脚
#define ECHO_PIN 7        // 超声波Echo引脚

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Connect to Wi-Fi
  WiFi.setHostname("MKR1010_Controller");
  connectToWiFi();

  // Set MQTT server
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("Setup complete");
}

void loop() {
  // Ensure connection to MQTT broker
  if (!client.connected()) {
    reconnectMQTT();
  }

  // Ensure connection to Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  // Process incoming MQTT messages
  client.loop();

  // Control LEDs using LDR value
  controlLightPattern();

  delay(500); // Small delay to avoid overwhelming the MQTT broker
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay(5000);
  }
  Serial.print("Connected to Wi-Fi. IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect("MKR1010_Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void controlLightPattern() {
  // Measure LDR value
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  // Map LDR value to the number of LEDs
  int numLEDs;
  if (ldrValue < 400) {
    numLEDs = 12; // 点亮所有灯
  } else if (ldrValue > 1000) {
    numLEDs = 0; // 熄灭所有灯
  } else {
    numLEDs = map(ldrValue, 1000, 400, 1, 12); // 在 400 到 1000 之间映射灯数量
    numLEDs = constrain(numLEDs, 0, 12); 
  }

  // Measure distance for color control
  float distance = measureDistance();

  // Update LEDs
  char mqtt_message[100];
  for (int i = 0; i < 12; i++) {
    int r = 0, g = 0, b = 0;

    if (i < numLEDs) {
      // 点亮的灯，控制颜色
      if (distance <= 30) {
        // 距离小于 30cm 时完全红色
        r = 255;
        g = 0;
        b = 0;
      } else {
        // 颜色渐变：距离远时偏蓝，近时偏红
        r = map(distance, 30, 100, 255, 0);
        g = 0;
        b = map(distance, 30, 100, 0, 255);
      }
    } // 未点亮的灯默认保持关闭状态

    sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": 0}", i, r, g, b);
    client.publish(mqtt_topic_pixels, mqtt_message);

    Serial.print("Published to pixel: ");
    Serial.println(mqtt_message);
  }
}

float measureDistance() {
  // 超声波测距
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2.0; // 转换为厘米
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}
