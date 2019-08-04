#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#include "constants.h"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

DHT dht(D4, DHT22, 11);

ADC_MODE(ADC_VCC);

void setup() {
  Serial.begin(115200);
  setup_esp8266();
  setup_device();
  setup_wifi();
  setup_mqtt();
  delay(2000);
}

void loop() {
  if (!mqtt.connected()) {
    mqttReconnect();
  }

  handle();

  mqtt.loop();

  enterDeepSleep();
}

void handle() {
  float currentTemp, currentHum;
  if (readValues(&currentTemp, &currentHum)) {
    printValues(currentTemp, currentHum);
    publishValues(currentTemp, currentHum);
  }
  else {
    Serial.println("Could not read values.");
  }
  publishStatus();
}

void setup_esp8266() {
  pinMode(D2, OUTPUT);
  pinMode(D4, INPUT);
}

void setup_device() {
  dht.begin();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(SSID);

  WiFi.begin(SSID, PSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  int32_t rssi = wifi_station_get_rssi();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Signal strength: ");
  Serial.print(rssi);
  Serial.println("dBm");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  mqtt.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
}

void mqttReconnect() {
  while (!mqtt.connected()) {
    Serial.print("Connecting MQTT at ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_BROKER_PORT);

    if (!mqtt.connect("ESP8266Client")) {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println("  retrying in 5 seconds");
      delay(5000);
    }
    else {
      Serial.println("MQTT Connected...");
    }
  }
}

bool readValues(float* temperature, float* humidity) {
  *temperature = dht.readTemperature();
  *humidity = dht.readHumidity();

  int counter = 0;
  while (counter < 10 && (isnan(*temperature) || isnan(*humidity))) {
    resetDHT22();
    *temperature = dht.readTemperature();
    *humidity = dht.readHumidity();
    counter++;
  }
  return !(isnan(*temperature) || isnan(*humidity));
}

void resetDHT22() {
  digitalWrite(D2, LOW);
  delay(1000);
  digitalWrite(D2, HIGH);
  delay(1000);
}

void printValues(float temperature, float humidity) {
  Serial.print("Temperature: ");
  Serial.print(String(temperature).c_str());
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(String(humidity).c_str());
  Serial.println("%");
}

void publishValues(float temperature, float humidity) {
  mqtt.publish("/test/dht22/temperature", String(temperature).c_str());
  mqtt.publish("/test/dht22/humidity", String(humidity).c_str());
}

void publishStatus() {
  int32_t rssi = wifi_station_get_rssi();
  mqtt.publish("/test/dht22/wifi-strength", String(rssi).c_str());

  uint adcValue = ESP.getVcc();
  float adc = adcValue / 1000.0;
  mqtt.publish("/test/dht22/battery", String(adc).c_str());
}


void enterDeepSleep() {
  delay(500);
  ESP.deepSleep(DEEP_SLEEP_INTERVAL);
  delay(100);
}
