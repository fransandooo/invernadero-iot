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
const char* mqtt_topic = "soil";

// --- OBJETOS ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- TICKER ---
Ticker wifiTicker;
void tickWifi() {
  Serial.print(".");
}

Ticker mqttTicker;
void tickMQTT() {}

Ticker soilloopTicker;
void tickSoilloop() {
  int soilValue = analogRead(A0);

  // Mapeo opcional (ajusta valores tras calibrar)
  int humedad = map(soilValue, 1023, 300, 0, 100);
  humedad = constrain(humedad, 0, 100);

  Serial.print("Humedad suelo: ");
  Serial.print(humedad);
  Serial.print("%  (raw: ");
  Serial.print(soilValue);
  Serial.println(")");

  // JSON
  StaticJsonDocument<200> doc;
  doc["humedad"] = humedad;
  doc["raw"] = soilValue;

  char buffer[128];
  serializeJson(doc, buffer);

  client.publish(mqtt_topic, buffer);

}

//  --- FUNCIONES ---
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
    if (client.connect("ESP8266_SUELO")) {
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
  client.setServer(mqtt_server, 1883);

  reconnect();
  soilloopTicker.attach_ms(3000, tickSoilloop); // publica cada 3 s
}

// --- LOOP ---
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  yield(); // recomendado para el ESP8266
}
