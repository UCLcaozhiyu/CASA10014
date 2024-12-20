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
const char* mqtt_topic_brightness = "student/CASA0014/light/3/brightness/";
const char* mqtt_topic_pixels = "student/CASA0014/light/3/pixel/";

// Sensor pins
#define LDR_PIN A0        // 光敏电阻引脚
#define TRIG_PIN 6        // 超声波Trig引脚
#define ECHO_PIN 7        // 超声波Echo引脚

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// 环形缓冲区配置
const int NUM_SAMPLES = 5; // 滑动平均的样本数量
float distanceBuffer[NUM_SAMPLES] = {0}; // 用于存储最近的距离测量值
int bufferIndex = 0; // 当前缓冲区索引

// 定时器配置
unsigned long lastCloseTime = 0; // 上次关闭灯的时间
const unsigned long CLOSE_INTERVAL = 1000; // 每次关闭灯的时间间隔（毫秒）
int currentLEDs = 12; // 当前点亮的 LED 数量

// 超声波测距相关
float previousDistance = -1; // 上一次的距离值
const float DISTANCE_THRESHOLD = 10.0; // 超声波距离变化的阈值（单位：cm）

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

  controlBrightness();
  // Control LED ring with distance
  controlLightPattern();

  delay(100); // Small delay to avoid overwhelming the MQTT broker
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
void controlBrightness() {
  int ldrValue = analogRead(LDR_PIN); // 读取光敏电阻值
  Serial.print("LDR: ");
  Serial.println(ldrValue);
  int brightness;
  if (ldrValue > 1000) {
    brightness = 0; // 超过 1000 时关闭灯
  } else if (ldrValue >= 300 && ldrValue <= 1000) {
    brightness = map(ldrValue, 1000, 300, 10, 118); // 映射到 50~118
  } else {
    brightness = 119; // 小于 300 时最大亮度
  } // 将光敏值映射到亮度范围
  // 构建亮度消息
  char mqtt_message[50];
  sprintf(mqtt_message, "{\"brightness\": %d}", brightness);

  // 发布亮度到 MQTT
  client.publish(mqtt_topic_brightness, mqtt_message);

  Serial.print("Published brightness: ");
  Serial.println(brightness);
}

void controlLightPattern() {
  float distance = measureDistance(); // 测量距离

  // 滑动平均滤波
  distanceBuffer[bufferIndex] = distance;
  bufferIndex = (bufferIndex + 1) % NUM_SAMPLES;

  // 计算滑动平均距离
  float smoothedDistance = calculateAverageDistance();

  // 如果距离变化超过阈值，立即点亮所有灯
  if (previousDistance < 0 || abs(smoothedDistance - previousDistance) > DISTANCE_THRESHOLD) {
    previousDistance = smoothedDistance; // 更新上一次的距离
    currentLEDs = 12; // 点亮所有灯

    char mqtt_message[100];
    int r, g, b;

  if (smoothedDistance <= 20) { // 红色表示近距离
    r = 255;
    g = 0;
    b = 0;
} else if (smoothedDistance > 100) { // 超出范围显示蓝色
    r = 0;
    g = 0;
    b = 255;
} else { // 距离 21 到 60 的渐变
    r = map(smoothedDistance, 21, 100, 255, 0);
    g = 0;
    b = map(smoothedDistance, 21, 100, 0, 255);
}


    for (int i = 0; i < currentLEDs; i++) {
      sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": 0}", i, r, g, b);
      client.publish(mqtt_topic_pixels, mqtt_message);
    }
  }

  // 逐个关闭灯
  unsigned long currentTime = millis();
  if (currentLEDs > 0 && currentTime - lastCloseTime >= CLOSE_INTERVAL) {
    lastCloseTime = currentTime;

    // 关闭一个灯
    char mqtt_message[100];
    currentLEDs--;
    sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": 0, \"G\": 0, \"B\": 0, \"W\": 0}", currentLEDs);
    client.publish(mqtt_topic_pixels, mqtt_message);
  }
}

float calculateAverageDistance() {
  float sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += distanceBuffer[i];
  }
  return sum / NUM_SAMPLES;
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
  return distance;
}
