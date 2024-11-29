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

// MQTT topic for your device
char mqtt_topic_1[] = "student/CASA0014/light/31/pixel/";
char mqtt_topic_2[] = "student/CASA0014/light/31/brightness/";
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

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

  // Send a color command to a specific pixel
  sendMQTTMessage();

  Serial.println("Message sent");
  delay(10000);
}

void connectToWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay(10000);
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

void sendMQTTMessage() {
  char mqtt_message[100];
  //for(int i=0; i<12; i++){
  //sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", i, 255, 0, 0, 0);
  sprintf(mqtt_message, "{\"brightness\": 50}");
  Serial.println(mqtt_topic_2);
  Serial.println(mqtt_message);

  if (client.publish(mqtt_topic_2, mqtt_message)) {
    Serial.println("Message published");
  } else {
    Serial.println("Failed to publish message");
  }
  //char mqtt_message[100];
  for(int i=0; i<12; i++){
  sprintf(mqtt_message, "{\"pixelid\": %d, \"R\": %d, \"G\": %d, \"B\": %d, \"W\": %d}", i, 255, 0, 0, 0);
  //sprintf(mqtt_message, "{\"brightness\": 1}");
  Serial.println(mqtt_topic_1);
  Serial.println(mqtt_message);

  if (client.publish(mqtt_topic_1, mqtt_message)) {
    Serial.println("Message published");
  } else {
    Serial.println("Failed to publish message");
  }
 // }
}
}


