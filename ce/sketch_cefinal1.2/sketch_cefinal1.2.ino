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
const char* mqtt_topic_brightness = "student/CASA0014/light/11/brightness/";
const char* mqtt_topic_pixels = "student/CASA0014/light/11/pixel/";

// Sensor pins
#define LDR_PIN A0        
#define TRIG_PIN 6       
#define ECHO_PIN 7        

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

  // Control brightness using LDR
  controlBrightness();

  // Control color and number of LEDs using distance
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

void controlBrightness() {
  int ldrValue = analogRead(LDR_PIN); 
  int brightness = map(ldrValue, 0, 1023, 0, 255); 

  char mqtt_message[50];
  sprintf(mqtt_message, "{\"brightness\": %d}", brightness);

  client.publish(mqtt_topic_brightness, mqtt_message);

  Serial.print("Published brightness: ");
  Serial.println(brightness);
}

void controlLightPattern() {
  float distance = measureDistance(); 
  int numLEDs = map(distance, 0, 100, 12, 0); 
  numLEDs = constrain(numLEDs, 0, 12);      

  char mqtt_message[100];
  for (int i = 0; i < 12; i++) {
    int r, g, b;

    if (distance <= 5) {
  
      r = 255;
      g = 0;
      b = 0;
    } else {
      r = map(distance, 5, 100, 255, 0);
      g = 0;
      b = map(distance, 5, 100, 0, 255);
    }

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
  return distance;
}
