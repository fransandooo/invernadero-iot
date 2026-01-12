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
const char* mqtt_topic = "ldr";

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- PIN ---
const int ldrPin = A0; // pin analógico conectado al LDR (el LDR lee entre 0 y 1023)

// --- TICKER ---
Ticker wifiTicker;
void tickWifi() {
  Serial.print(".");
}

Ticker mqttTicker;
void tickMQTT() {}

Ticker ldrloopTicker;
void tickLDRloop() {
  int valor = analogRead(ldrPin); // 0-1023
  int luz = map(valor, 0, 1023, 0, 100); // 0% = oscuro, 100% = luz máxima
  // a mayor raw, mayor porcentaje de luz
  luz = constrain(luz, 0, 100);

  Serial.print("Luz: ");
  Serial.print(luz);
  Serial.print("%  (raw: ");
  Serial.print(valor);
  Serial.println(")");

  StaticJsonDocument<100> doc;
  doc["luz"] = luz;
  doc["raw"] = valor;

  char buffer[64];
  serializeJson(doc, buffer);
  client.publish(mqtt_topic, buffer);
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
    if (client.connect("ESP8266_LDR")) {
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
  ldrloopTicker.attach_ms(3000, tickLDRloop); // publica cada 3 s
}

// --- LOOP ---
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  yield(); // recomendado para el ESP8266
}
