
// Este sensor (DHT11) lee la temperatura y la humedad ambiente y envía los datos a un broker MQTT cada 3 segundos

// --- INCLUDES ---
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// --- WIFI ---
// todos los nodos y ordenadores tienen que estar conectados a la misma red
const char* ssid = "WWWWW"; // cambiar por el nombre de la red WiFi 
const char* password = "w.w.w.w.w"; // cambiar por la contraseña de la red WiFi

// --- MQTT ---
const char* mqtt_server = "10.228.245.75"; //donde está levantado
const char* mqtt_topic = "dht11";

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- DHT11 ---
#define DHTPIN GPIO_ID_PIN(2)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); // Definimos el sensor DHT11 con el pin y tipo definidos previamente

// --- TICKER ---
Ticker wifiTicker;
void tickWifi() {
  Serial.print(".");
}

Ticker mqttTicker;
void tickMQTT() {}

Ticker dht11loopTicker;
void tickDHT11loop() {
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
}


// --- FUNCIONES ---
// Conexión a la red WiFi
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  wifiTicker.attach_ms(500, tickWifi); // Imprime un punto cada 500 ms para indicar que está intentando conectar
  // Esperamos a que conete
  while (WiFi.status() != WL_CONNECTED) {
    yield(); // recomendado en ESP8266
  }
  wifiTicker.detach(); // Detenemos el ticker cuando ya se ha conectado a la wifi

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Conexión al broker MQTT
void reconnect() {
  Serial.print("Conectando MQTT...");

  mqttTicker.attach_ms(3000, tickMQTT); // cada 3 segundos reintenta conectar

  while (!client.connected()) {
    if (client.connect("NodeMCU_DHT11")) {
      Serial.println(" conectado");
    } else {
      Serial.print(" fallo rc=");
      Serial.println(client.state());
    }
    yield(); // recomendado en ESP8266
  }

  mqttTicker.detach(); // detenemos el ticker al conectar
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("Test DHT11");
  dht.begin();
  setup_wifi(); // Conectar a WiFi
  client.setServer(mqtt_server, 1884); // Configurar broker MQTT

  reconnect(); // Conectar a MQTT
  dht11loopTicker.attach_ms(3000, tickDHT11loop); // publica cada 3 s
}

// --- LOOP ---
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  yield(); // recomendado para el ESP8266
}