/*lee la humedad y temperatura */


#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// --- WIFI ---
const char* ssid = "WWWWW"; // ssid de la wifi a la que el nodo se conecte (igual que a la que el pc esté conectada)
const char* password = "w.w.w.w.w";

// --- MQTT ---
const char* mqtt_server = "10.228.245.191"; // IP real del PC en la red del hotspot
const char* mqtt_topic = "personaA/dht11";

WiFiClient espClient;
PubSubClient client(espClient);

// --- PROTOTIPOS ---
void setup_wifi();
void reconnect();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Test DHT11");
  dht.begin();

  setup_wifi();              // Conectar a WiFi
  client.setServer(mqtt_server, 1883);  // Configurar broker MQTT
}

void loop() {
  if (!client.connected()) {
    reconnect();             // Reconectar si se cae MQTT
  }
  client.loop();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo DHT11");
  } else {
    Serial.print("Humedad: ");
    Serial.print(h);
    Serial.print(" %  |  Temp: ");
    Serial.print(t);
    Serial.println(" C");

    // Crear JSON
    StaticJsonDocument<200> doc;
    doc["temperatura"] = t;
    doc["humedad"] = h;
    char buffer[128];
    size_t n = serializeJson(doc, buffer);

    // Publicar MQTT
    client.publish(mqtt_topic, buffer, n);
  }

  delay(2000);
}

// --- DEFINICIÓN DE FUNCIONES ---
void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 20 intentos de 500ms = 10s
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo se pudo conectar a WiFi");
  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect("NodeMCU_DHT11")) {
      Serial.println("Conectado");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}