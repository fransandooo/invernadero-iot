// --- INCLUDES ---
// estas librerías se incluyen para el funcionemiento básico de Arduino, para la conexión WiFi, para el uso de MQTT, para crear y editar 
//    Json, y para poder ejecutar funciones periódicamente usando temporizadores (sin el uso de delay o millis).
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// --- WIFI ---
// todos los nodos y ordenadores tienen que estar conectados a la misma red
const char* ssid = "wifi"; // cambiar por el nombre de la red WiFi 
const char* password = "wifiwifi"; // cambiar por la contraseña de la red WiFi

// --- MQTT ---
const char* mqtt_server = "10.228.245.75"; //la IP de donde esté levantado
const char* mqtt_topic = "ldr"; // el topic es donde se publican los datos del sensor

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- PIN ---
const int ldrPin = A0; // pin analógico conectado al LDR (el LDR lee entre 0 y 1023)

// --- TICKER ---
Ticker wifiTicker; // definimos el ticker que usaremos para la WiFi
void tickWifi() {
  Serial.print("."); // esta función imprime un punto cada cierto tiempo mientras el nodo intenta conectarse a la WiFi (es solo informativo, para saber que no se ha bloqueado).
}

Ticker mqttTicker; // definimos el ticker que usaremos para el MQTT
void tickMQTT() {} // aquí no imprimimos un punto, pero tiene la misma funcionalidad que el ticker de la WiFi --> para reintentar la conexión MQTT periódicamente.

Ticker ldrloopTicker; // definimos el ticker que usaremos para el loop 
void tickLDRloop() {
  int valor = analogRead(ldrPin); // Hacemos la lectura del sensor, que mide entre 0 y 1023
  // hacemos una conversión de la lectura a porcentaje
  int luz = map(valor, 0, 1023, 0, 100); // 0% = oscuro, 100% = luz máxima
  // a mayor raw, mayor porcentaje de luz
  luz = constrain(luz, 0, 100); // limita que el valor de luz se mantengan en rango

  //imprimimos por el Serial Monitor luz (el porcentaje convertido del valor raw) y el raw (el valor leido, comprendido entre 0 y 1023)
  Serial.print("Luz: ");
  Serial.print(luz);
  Serial.print("%  (raw: ");
  Serial.print(valor);
  Serial.println(")");

  StaticJsonDocument<100> doc; // creamos el objeto JSON en memoria
  doc["luz"] = luz;
  doc["raw"] = valor;

  // convertimos el JSON a texto y publicamos el mensaje en el topic
  char buffer[64];
  serializeJson(doc, buffer);
  client.publish(mqtt_topic, buffer);
}


// --- FUNCIONES ---
// Conexión a la red WiFi
void setup_wifi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password); // iniciamos la conexión WiFi

  wifiTicker.attach_ms(500, tickWifi); // llamamos a tickWifi() cada 500ms, que imprimirá un punto para indicar que está intentando conectar
  // Esperamos a que conete sin bloquear el sistema
  while (WiFi.status() != WL_CONNECTED) {
    yield(); // entre otros, resetea el watchdog y permite que el ESP8266 ejecute tareas internamente
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
      Serial.println(client.state()); // si falla en conectar, aquí nos da el código de error
    }
    yield(); // recomendado en ESP8266
  }

  mqttTicker.detach(); // detenemos el ticker al conectar
}

// --- SETUP ---
void setup() {
  // inicializamos
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1884); // configuramos el MQTT al  puerto 1884

  reconnect(); // llamamos a la función para conectar MQTT
  ldrloopTicker.attach_ms(3000, tickLDRloop); // ejecutamos la lectura del sensor y publica cada 3 s
}

// --- LOOP ---
void loop() {
  if (!client.connected()) reconnect(); // verificamos que la conexión MQTT no ha caído
  client.loop();

  yield(); // recomendado para el ESP8266 ya que evita reinicios por watchdog en el ESP8266 (el watchdog reinicia el programa si éste se queda colgado/congelado)
}
