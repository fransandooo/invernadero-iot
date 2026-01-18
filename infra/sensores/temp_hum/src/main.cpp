// --- INCLUDES ---
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// // --- WIFI ---
const char* ssid = "Gonzalos S24";
const char* password = "bondia2020";

// // --- MQTT ---
const char* mqtt_server = "10.180.73.213";
const char* mqtt_topic = "dht11";

// --- OBJETOS ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- DHT11 ---
#define DHTPIN GPIO_ID_PIN(2)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- FLAGS VOLÁTILES ---
volatile bool shouldReadDHT = false;
volatile bool shouldCheckWifi = false;
volatile bool shouldCheckMqtt = false;

// --- TICKERS ---
Ticker dht11Ticker;
Ticker wifiCheckTicker;
Ticker mqttCheckTicker;

// --- ESTADOS ---
bool wifiConnected = false;
bool mqttConnected = false;

// --- CALLBACKS MÍNIMOS (solo setean flags) ---
void IRAM_ATTR onDHTTimer() {
  shouldReadDHT = true;
}

void IRAM_ATTR onWifiCheck() {
  shouldCheckWifi = true;
}

void IRAM_ATTR onMqttCheck() {
  shouldCheckMqtt = true;
}

// --- FUNCIONES ---
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Iniciar ticker para verificar WiFi cada 500ms
  wifiCheckTicker.attach_ms(500, onWifiCheck);
}

void checkWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      wifiCheckTicker.detach();
      
      Serial.println("\nWiFi conectado");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      // Configurar MQTT después de conectar WiFi
      client.setServer(mqtt_server, 1884);
      
      // Iniciar intento de conexión MQTT
      mqttCheckTicker.attach_ms(3000, onMqttCheck);
      shouldCheckMqtt = true; // Intentar inmediatamente
    }
  } else {
    wifiConnected = false;
    Serial.print(".");
  }
}

void checkMqttConnection() {
  if (!wifiConnected) {
    return; // No intentar si no hay WiFi
  }
  
  if (client.connected()) {
    if (!mqttConnected) {
      mqttConnected = true;
      mqttCheckTicker.detach();
      Serial.println("MQTT conectado");
      
      // Iniciar lectura del DHT11 cada 3 segundos
      dht11Ticker.attach(3, onDHTTimer);
      shouldReadDHT = true; // Primera lectura inmediata
    }
  } else {
    mqttConnected = false;
    Serial.print("Conectando MQTT...");
    
    if (client.connect("NodeMCU_DHT11")) {
      Serial.println(" OK");
    } else {
      Serial.print(" fallo rc=");
      Serial.println(client.state());
    }
  }
}

void readAndPublishDHT() {
  if (!mqttConnected) {
    return; // No leer si no hay conexión MQTT
  }
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo DHT11");
    return;
  }
  
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print(" %  |  Temp: ");
  Serial.print(t);
  Serial.println(" C");
  
  // Crear JSON
  StaticJsonDocument<128> doc;
  doc["temperatura"] = t;
  doc["humedad"] = h;
  
  char buffer[128];
  serializeJson(doc, buffer);
  
  // Publicar MQTT
  if (client.publish(mqtt_topic, buffer)) {
    Serial.println("✓ Publicado");
  } else {
    Serial.println("✗ Error publicando");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("\nTest DHT11");
  
  dht.begin();
  setup_wifi();
}

// --- LOOP ---
void loop() {
  // Procesar flags de los tickers
  
  if (shouldCheckWifi) {
    shouldCheckWifi = false;
    checkWifiStatus();
  }
  
  if (shouldCheckMqtt) {
    shouldCheckMqtt = false;
    checkMqttConnection();
  }
  
  if (shouldReadDHT) {
    shouldReadDHT = false;
    readAndPublishDHT();
  }
  
  // Mantener MQTT vivo
  if (mqttConnected) {
    client.loop();
  }
  
  yield();
}