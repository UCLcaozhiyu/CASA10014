#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "arduino_secrets.h"

// Wi-Fi and MQTT credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = "mqtt.cetools.org";
const int mqtt_port = 1884;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;

// MQTT topics
const char* mqtt_topic_brightness = "student/CASA0014/light/2/brightness/";
const char* mqtt_topic_pixels = "student/CASA0014/light/2/pixel/";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// NeoPixel configuration
#define PIN 6
#define NUMPIXELS 12
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Sensor pins
#define LDR_PIN A0        // 光敏电阻引脚
#define TRIG_PIN 8        // 超声波Trig引脚
#define ECHO_PIN 9        // 超声波Echo引脚

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize NeoPixel
  pixels.begin();

  // Connect to Wi-Fi
  WiFi.setHostname("123");
  connectToWiFi();

  // Set MQTT server and callback
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

  // Adjust brightness based on light sensor
  adjustBrightness();

  // Adjust light pattern based on distance
  adjustLightsByDistance();

  delay(100);
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
    if (client.connect("ArduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void adjustBrightness() {
  int ldrValue = analogRead(LDR_PIN);
  int brightness = map(ldrValue, 0, 1023, 0, 120); // 限制最大亮度为120
  char mqtt_message[50];
  sprintf(mqtt_message, "{\"brightness\": %d}", brightness);
  client.publish(mqtt_topic_brightness, mqtt_message);

  pixels.setBrightness(brightness);
  pixels.show();

  Serial.print("Brightness adjusted to: ");
  Serial.println(brightness);
}

void adjustLightsByDistance() {
  float distance = measureDistance();
  int numLEDs = map(distance, 0, 50, NUMPIXELS, 0); // 半米内全亮，逐渐递减
  if (numLEDs < 0) numLEDs = 0; // 防止负数

  for (int i = 0; i < NUMPIXELS; i++) {
    if (i < numLEDs) {
      pixels.setPixelColor(i, pixels.Color(0, 255 - (i * 20), i * 20)); // 渐变颜色
    } else {
      pixels.setPixelColor(i, 0); // 关闭灯
    }
  }
  pixels.show();

  char mqtt_message[200];
  sprintf(mqtt_message, "{\"pixels\": [");
  for (int i = 0; i < NUMPIXELS; i++) {
    int r = (i < numLEDs) ? 0 : 0;
    int g = (i < numLEDs) ? 255 - (i * 20) : 0;
    int b = (i < numLEDs) ? i * 20 : 0;
    sprintf(mqtt_message + strlen(mqtt_message), "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d}", i, r, g, b);
    if (i < NUMPIXELS - 1) strcat(mqtt_message, ", ");
  }
  strcat(mqtt_message, "]}");
  client.publish(mqtt_topic_pixels, mqtt_message);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm, LEDs lit: ");
  Serial.println(numLEDs);
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2.0; // Convert to cm
  return distance;
}
