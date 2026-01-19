// --- INCLUDES ---
// estas librerías se incluyen para el funcionemiento básico de Arduino, para la conexión WiFi, para la comunicación mediante MQTT, para crear y gestionar mensajes en formato JSON,
//   y para poder ejecutar funciones de forma periódica mediante temporizadores sin bloquear el programa (evitando el uso de delay o millis).
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// --- WIFI ---
// todos los nodos tienen que estar conectados a la misma red
const char* ssid = "wifi"; // cambiar por el nombre de la red WiFi 
const char* password = "wifiwifi"; // cambiar por la contraseña de la red WiFi

// --- MQTT ---
const char* mqtt_server = "10.228.245.75"; // la IP de donde esté levantado
const char* mqtt_topic = "soil"; // el topic es donde se publican los datos del sensor

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- TICKER ---
Ticker wifiTicker; // definimos el ticker que usaremos para la WiFi
void tickWifi() {
  Serial.print("."); // esta función imprime un punto cada cierto tiempo mientras el nodo intenta conectarse a la WiFi (es solo informativo, para saber que no se ha bloqueado).
}

Ticker mqttTicker; // definimos el ticker que usaremos para el MQTT
void tickMQTT() {} // aquí no imprimimos un punto, pero tiene la misma funcionalidad que el ticker de la WiFi --> para reintentar la conexión MQTT periódicamente.

Ticker soilloopTicker; // definimos el ticker que usaremos para el loop
void tickSoilloop() {
  int soilValue = analogRead(A0); // Hacemos la lectura del sensor, que mide entre 0 y 1023
  int humedad = map(soilValue, 1023, 0, 0, 100);  // hacemos una conversión de la lectura a porcentaje
  humedad = constrain(humedad, 0, 100); // limita que el valor de luz se mantengan en rango

  // imprimimos por el Serial Monitorla humedad (el porcentaje convertido del valor raw) y el raw (el valor leido)
  Serial.print("Humedad suelo: ");
  Serial.print(humedad);
  Serial.print("%  (raw: ");
  Serial.print(soilValue);
  Serial.println(")");

  StaticJsonDocument<200> doc;  // creamos el objeto JSON en memoria
  doc["humedad"] = humedad;
  doc["raw"] = soilValue;

  // convertimos el JSON a texto y publicamos el mensaje en el topic
  char buffer[128];
  serializeJson(doc, buffer);
  client.publish(mqtt_topic, buffer);

}

//  --- FUNCIONES ---
// Conexión a la red WiFi
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // iniciamos la conexión WiFi

  wifiTicker.attach_ms(500, tickWifi);  // llamamos a tickWifi() cada 500ms, que imprimirá un punto para indicar que está intentando conectar
  // Esperamos a que conete sin bloquear el sistema
  while (WiFi.status() != WL_CONNECTED) {
    yield(); // recomendado en ESP8266 ya que, entre otros, resetea el watchdog y permite que el ESP8266 ejecute tareas internamente
  }
  wifiTicker.detach(); // Detenemos el ticker cuando ya se ha conectado a la wifi

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// // Conexión al broker MQTT
void reconnect() {
  Serial.print("Conectando MQTT...");

  mqttTicker.attach_ms(3000, tickMQTT); // cada 3 segundos reintenta conectar

  while (!client.connected()) {
    if (client.connect("ESP8266_SUELO")) {
      Serial.println(" conectado");
    } else {
      Serial.print(" fallo rc=");
      Serial.println(client.state()); // si falla en conectar, aquí nos da el código de error
    }
    yield(); // recomendado en ESP8266
  }

  mqttTicker.detach(); // detenemos el ticker al conectar
}

// // --- SETUP ---
void setup() {
  // inicializamos
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1884); // configuramos el MQTT al  puerto 1884

  reconnect(); // llamamos a la función para conectar MQTT
  soilloopTicker.attach_ms(3000, tickSoilloop); // ejecutamos la lectura del sensor y publica cada 3 s
}

// // --- LOOP ---
void loop() {
  if (!client.connected()) reconnect(); // verificamos que la conexión MQTT no ha caído
  client.loop();

  yield(); // recomendado para el ESP8266 ya que evita reinicios por watchdog en el ESP8266 (el watchdog reinicia el programa si éste se queda colgado/congelado)
}
