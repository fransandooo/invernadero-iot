# Invernadero Inteligente IoT (Distribuido)

Proyecto final de la asignatura de IoT: un invernadero inteligente, distribuido y basado en eventos, implementado con nodos ESP8266 (NodeMCU), comunicación MQTT, y orquestación en Node-RED. El sistema procesa datos de sensores, toma decisiones automáticas (riego e iluminación), envía órdenes a actuadores, genera alertas (Telegram) y almacena históricos (ThingSpeak). Incluye dashboard web en Node-RED y posible integración con Alexa.

## Objetivos

- Arquitectura IoT realista, distribuida y escalable.
- Comunicación desacoplada mediante MQTT, sin `delay` ni código bloqueante.
- Procesamiento de eventos en Node-RED (reglas, automatizaciones y orquestación).
- Integraciones: Telegram (alertas y control), Dashboard, ThingSpeak, Alexa (opcional).

## Arquitectura

- Nodos IoT (ESP8266/NodeMCU):
  - Sensores: temperatura/humedad ambiental, humedad de suelo, iluminación, nivel de agua, etc.
  - Actuadores: riego (bomba/solenoide), iluminación (relé/LED), ventilación, etc.
- Broker MQTT (Eclipse Mosquitto): canal de eventos entre dispositivos y backend.
- Backend de orquestación (Node-RED):
  - Ingesta y normalización de datos de los nodos.
  - Reglas y lógica de control (riego/iluminación/alertas).
  - Integraciones: Telegram, ThingSpeak, Dashboard y Alexa (opcional).

Diagrama lógico simplificado:

```
ESP8266 Nodes  <-->  MQTT Broker (1883)  <-->  Node-RED (1880)
																					 |--> Dashboard
																					 |--> Telegram
																					 |--> ThingSpeak
																					 |--> Alexa
																					 |--> Google Assistant
```

## Estructura del repositorio

```
README.md
docs/
infra/
	docker-compose.yml
	mosquitto/
		config/
			mosquitto.conf
		data/
		log/
	nodered/
		data/
			package.json
			settings.js
			lib/
				flows/
```

- `infra/docker-compose.yml`: orquesta Mosquitto y Node-RED.
- `infra/mosquitto/config/mosquitto.conf`: configuración del broker MQTT.
- `infra/nodered/data/`: datos de runtime de Node-RED (flujos, credenciales, librería, etc.).
- `docs/`: documentación funcional/técnica, esquemas, notas, etc.

## Requisitos

- Docker y Docker Compose.
- Nodos ESP8266 (NodeMCU) con firmware Arduino/PlatformIO usando MQTT.

## Puesta en marcha (infraestructura)

1. Levanta Mosquitto y Node-RED con Docker Compose:

```bash
cd infra
docker compose up -d
```

2. Accede a los servicios:

- Node-RED: http://localhost:1880
- Broker MQTT: `mqtt://localhost:1883`

3. (Opcional) Ver logs:

```bash
docker compose logs -f mosquitto
docker compose logs -f nodered
```

## Configuración en Node-RED

Abre el editor (http://localhost:1881) y revisa/crea los flujos:

- Instalación de nodos recomendados (Menú > Manage palette > Install):

  - `node-red-dashboard` (UI web en `/ui`).
  - `node-red-node-telegram` o `node-red-contrib-telegrambot` (bot de Telegram).
  - `node-red-contrib-thingspeak` (o integración HTTP a la API de ThingSpeak).

- Credenciales y configuraciones:

  - Telegram: token del bot (BotFather) y chat IDs.
  - ThingSpeak: API Key del canal y campos configurados.
  - MQTT: broker en `mqtt://mosquitto:1883` desde Node-RED (dentro de Docker) o `mqtt://localhost:1883` desde fuera.

- Dashboard:
  - Tras instalar `node-red-dashboard`, la UI queda disponible en: http://localhost:1880/ui
  - Añade gráficos, indicadores y controles (riego/iluminación manual) según los tópicos definidos.

## Convenciones MQTT

Sugerencia de espacios de nombres y cargas útiles para mantener consistencia y escalabilidad:

- Topics (ejemplo):

  - Telemetría sensores: `invernadero/sensor/{nodo}/{tipo}`
    - `tipo`: `temp`, `hum`, `suelo`, `luz`, `nivel`, `gas`...
  - Estados actuadores: `invernadero/act/{nodo}/{dispositivo}` (publicado por el nodo tras actuar)
  - Órdenes a actuadores: `invernadero/cmd/{nodo}/{dispositivo}` con payload de comando
  - Alertas (backend -> usuarios): `invernadero/alert/{nivel}` (`info|warn|crit`)

- Payloads JSON (ejemplos):

```json
// Medida de sensor
{
	"ts": 1734390000000,
	"value": 23.6,
	"unit": "°C"
}

// Orden a actuador (riego)
{
	"cmd": "on",
	"seconds": 10,
	"reason": "soil<30%"
}

// Estado de actuador reportado por el nodo
{
	"state": "on",
	"remaining": 7
}
```

Recomendaciones:

- Usar QoS 1 para órdenes y eventos críticos; QoS 0 para telemetría frecuente.
- Incluir timestamp `ts` en milisegundos cuando aplique.
- Evitar retener (`retain`) salvo para últimos estados de actuadores.

## Nodos ESP8266 (firmware)

- Librerías habituales: `ESP8266WiFi`, `PubSubClient`, sensores (DHT, etc.).
- Conexión al broker: `mqtt://<IP_del_host>:1883` (o IP del contenedor en red Docker).
- Diseño no bloqueante: uso de callbacks y temporizadores en lugar de `delay`.
- Separar lógica de lectura de sensores, publicación MQTT y gestión de comandos.

## ThingSpeak

- Configura API Key y Channel ID en los nodos de ThingSpeak o mediante nodos HTTP.
- Mapea campos (`field1..field8`) a las variables de telemetría que quieras persistir.
- Considera agregaciones (media/mín/max) en Node-RED antes de publicar si necesitas reducir frecuencia.

## Telegram

- Crea un bot con BotFather y obtén el `TOKEN`.
- En Node-RED, añade nodos de Telegram para:
  - Alertas automáticas (condiciones críticas: temperatura alta, depósito vacío, etc.).
  - Comandos manuales: `riego on/off`, `iluminacion on/off`, `estado`, etc.

## Seguridad y despliegue

- Actualmente el editor no está protegido (sin `adminAuth`). En producción:
  - Habilita `adminAuth` en `infra/nodered/data/settings.js`.
  - Protege Mosquitto con usuarios/ACL en `mosquitto.conf`.
  - Expón servicios tras un proxy (TLS) o manténlos en red interna.

## Desarrollo de flujos

- Los flujos se almacenan en `infra/nodered/data/` (por defecto `flows.json` y `flows_cred.json`).
- Exporta/Importa desde el editor de Node-RED para versionar cambios en `docs/` o una carpeta dedicada.

## Solución de problemas

- Puertos en uso:
  - Node-RED: 1880, Mosquitto: 1883. Ajusta mapeos en `docker-compose.yml` si hay conflictos.
- Conectividad MQTT:
  - Desde Node-RED (en Docker) usa host `mosquitto` (nombre de servicio), puerto `1883`.
  - Desde dispositivos físicos, usa la IP/host de tu máquina que ejecuta Docker.
- Permisos de carpetas:
  - Asegura que `infra/nodered/data` y `infra/mosquitto/*` sean accesibles por Docker.

## Roadmap

- Añadir flujos de ejemplo preconfigurados (riego, alertas, dashboard y ThingSpeak).
- Añadir integración con Alexa (vía `node-red-contrib-alexa-home-skill` u opcional).
- Scripts de despliegue y backups de flujos.

## Licencia

Por definir.

## Créditos

Proyecto académico desarrollado por el equipo 2 del curso de IoT.
