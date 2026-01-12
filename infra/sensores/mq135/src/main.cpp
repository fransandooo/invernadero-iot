/* 
El sensor MQ135 detecta gases nocivos o contaminantes.
El sensor nos dice si hay gases que podrían dañar plantas o humanos

Raw: refleja la resistencia del sensor, que cambia según los gases presentes.
    Interpretación: cuanto más alto el valor, menor concentración de gases nocivos (dependiendo de la configuración y el voltaje).
Porcentaje: escala 0–100 que simplemente convierte la lectura cruda a algo más fácil de entender.
    Qué significa: no es un porcentaje real de CO₂ ni de gases, solo un indicador de “más limpio” o “más contaminado”. 
      Cerca de 0 → aire más contaminado, Cerca de 100 → aire más limpio  
*/

// --- INCLUDES ---
#include <Arduino.h>
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
const char* mqtt_topic = "mq135";

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- PIN ---
const int MQ135_PIN = A0; // pin analógico conectado al MQ135 (el MQ135 lee entre 0 y 1023)

// --- TICKER ---
Ticker wifiTicker;
void tickWifi() {
  Serial.print(".");
}

Ticker mqttTicker;
void tickMQTT() {}

Ticker mq135loopTicker;
void tickMQ135loop() {
  // --- Leer MQ135 ---
  int rawValue = analogRead(MQ135_PIN); // Valor crudo 0-1023
  int gasPercentage = map(rawValue, 0, 1023, 0, 100); // Conversión a "porcentaje"
  gasPercentage = constrain(gasPercentage, 0, 100);

  // --- Imprimir por Serial ---
  Serial.print("MQ135 raw: ");
  Serial.print(rawValue);
  Serial.print(" V | Concentración aprox: ");
  Serial.print(gasPercentage);
  Serial.println(" %");

  // --- Crear JSON ---
  StaticJsonDocument<128> doc;
  doc["raw"] = rawValue;
  doc["percentage"] = gasPercentage;

  char buffer[128];
  serializeJson(doc, buffer);

  // --- Publicar en MQTT ---
  if (client.publish(mqtt_topic, buffer)) {
    //Serial.println("Mensaje MQTT enviado correctamente");
  } else {
    Serial.println("Error enviando MQTT");
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
    if (client.connect("ESP8266_MQ135")) {
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
  setup_wifi();
  client.setServer(mqtt_server, 1884);

  reconnect();
  mq135loopTicker.attach_ms(3000, tickMQ135loop); // publica cada 3 s
}

// --- LOOP ---
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  yield(); // recomendado para el ESP8266
}