// --- INCLUDES ---
// estas librerías se incluyen para el funcionemiento básico de Arduino, una librería específica para el sensor utilizado (DHT11), otra para la conexión WiFi, 
//   para la conexión con MQTT, para crear y gestionar mensajes Json, y para poder ejecutar funciones periódicamente usando temporizadores (sin el uso de delay o millis).
#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// // --- WIFI ---
// todos los nodos tienen que estar conectados a la misma red
const char* ssid = "wifi"; // cambiar por el nombre de la red WiFi 
const char* password = "wifiwifi"; // cambiar por la contraseña de la red WiFi

// // --- MQTT ---
const char* mqtt_server = "10.228.245.75"; //la IP de donde esté levantado
const char* mqtt_topic = "dht11"; // el topic es donde se publican los datos del sensor

// --- OBJETOS ---
WiFiClient espClient; // definimos el cliente WiFi para la conexión MQTT
PubSubClient client(espClient); // definimos el cliente MQTT usando el cliente WiFi

// --- DHT11 ---
// definimos el pin digital donde se conecta el sensor DHT11 a nuestro nodo (GPIO2 del ESP8266)
#define DHTPIN GPIO_ID_PIN(2)
// indicamos el tipo de sensor DHT que estamos usando (DHT11)
#define DHTTYPE DHT11
// creamos el objeto DHT indicando el pin de conexión usado y el tipo de sensor (DHT11)
DHT dht(DHTPIN, DHTTYPE);

// --- FLAGS ---
// estas flags se activan desde los tickers y se gestionan en el loop principal
volatile bool shouldReadDHT = false; // indica cuándo se debe leer el sensor DHT11
volatile bool shouldCheckWifi = false; // indica cuándo se debe comprobar el estado del WiFi
volatile bool shouldCheckMqtt = false; // indica cuándo se debe comprobar la conexión MQTT

// --- TICKERS ---
Ticker dht11Ticker; // este ticker es para la lectura periódica del DHT11
Ticker wifiCheckTicker; // este ticker lo usamos para comprobar el estado de la conexión WiFi
Ticker mqttCheckTicker; // este ticker lo usamos para comprobar y reintentar (si es necesario) la conexión MQTT

// --- ESTADOS ---
// estas variables indican si actualmente hay conexión Wifi y MQTT activa
bool wifiConnected = false; 
bool mqttConnected = false;

// --- CALLBACKS MÍNIMOS ---
// estas funciones se ejecutan desde los tickers y solo activan flags (no se realizan otras operaciones dentro del callback)
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
  
  WiFi.mode(WIFI_STA); // configuramos el ESP8266 en modo estación
  WiFi.begin(ssid, password); // inicializamosmos la conexión
  
  // iniciamos un ticker que comprobará el estado del WiFi cada 500 ms
  wifiCheckTicker.attach_ms(500, onWifiCheck);
}

void checkWifiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true; // marcamos el estado WiFi como conectado
      wifiCheckTicker.detach(); // detenemos el ticker ya que la WiFi está conectada
      
      Serial.println("\nWiFi conectado");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      // configuramos el broker MQTT una vez conectado el WiFi, con el puerto 1884
      client.setServer(mqtt_server, 1884);
      
      // iniciamos el ticker para comprobar la conexión MQTT
      mqttCheckTicker.attach_ms(3000, onMqttCheck);
      shouldCheckMqtt = true; // seteamos la flag a true, indicando la conexión inmediata
    }
  } else {
    wifiConnected = false; // indicamos que no hay conexión WiFi
    Serial.print("."); // imprimimos un punto para señalizar visualmente el intento de conexión
  }
}

void checkMqttConnection() {
  if (!wifiConnected) {
    return; // no intentamos conectar MQTT si no hay WiFi
  }
  
  if (client.connected()) {
    if (!mqttConnected) {
      mqttConnected = true; // marcamos el estado MQTT como conectado
      mqttCheckTicker.detach(); // detenemos el ticker de comprobación MQTT
      Serial.println("MQTT conectado");
      
      // iniciamos la lectura del DHT11 cada 3 segundos
      dht11Ticker.attach(3, onDHTTimer);
      shouldReadDHT = true; // ponemos la flag a true indicando la primera lectura inmediata
    }
  } else {
    mqttConnected = false; // indicamos que no hay conexión MQTT
    Serial.print("Conectando MQTT...");
    
    if (client.connect("NodeMCU_DHT11")) {
      Serial.println(" OK");
    } else {
      Serial.print(" fallo rc=");
      Serial.println(client.state()); // si falla en conectar, aquí nos da el código de error
    }
  }
}

void readAndPublishDHT() {
  if (!mqttConnected) {
    return; // no leemos el sensor si no hay conexión MQTT
  }
  
  float h = dht.readHumidity(); // lectura de la humedad
  float t = dht.readTemperature(); // lectura de la temperatura

  // comprobamos si la lectura es válida
  if (isnan(h) || isnan(t)) {
    Serial.println("Error leyendo DHT11");
    return;
  }

  // mostramos los valores de humedad y temperatura por el Serial Monitor
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print(" %  |  Temp: ");
  Serial.print(t);
  Serial.println(" C");
  
  // creamos el objeto JSON con los valores leidos del sensor (temperatura y humedad)
  StaticJsonDocument<128> doc;
  doc["temperatura"] = t;
  doc["humedad"] = h;

  // convertimos el JSON a texto
  char buffer[128];
  serializeJson(doc, buffer);
  
  // Publicar MQTT
  if (client.publish(mqtt_topic, buffer)) {
    Serial.println("Publicado"); // mensaje de verificación de publicación exitosa
  } else {
    Serial.println("Error publicando"); // mensaje de verificación de publicación fallida
  }
}

// --- SETUP ---
void setup() {
  // inicializamos la comunicación serial
  Serial.begin(115200);
  Serial.println("\nTest DHT11");
  // inicializamos el sensot DHT11
  dht.begin();
  // inicializamos la conexión WiFi
  setup_wifi();
}

// --- LOOP ---
void loop() {
  // dentro del loop, en primer lugar, procesamos las acciones pendientes indicadas por los tickers  
  if (shouldCheckWifi) {
    shouldCheckWifi = false;
    checkWifiStatus(); // comprobamos el estado de la WiFi
  }
  
  if (shouldCheckMqtt) {
    shouldCheckMqtt = false;
    checkMqttConnection(); // comprobamos la conexión MQTT
  }
  
  if (shouldReadDHT) {
    shouldReadDHT = false;
    readAndPublishDHT(); // leemos el sensor y publicamos los datos
  }
  
  // También confirmamos que la conexión MQTT se mantiene viva
  if (mqttConnected) {
    client.loop();
  }
  
  yield(); // recomendado para el ESP8266, ya que evita bloqueos y reinicios por watchdog
}
