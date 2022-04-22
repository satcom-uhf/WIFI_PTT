#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <driver/dac.h>

#define LED_BUILTIN 2
#define HTTP_PORT 80

// WiFi credentials
const char *WIFI_SSID = "sharp-dev.net";
const char *WIFI_PASS = "satcom73";
void initSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1)
      digitalWrite(LED_BUILTIN, millis() % 200 < 50 ? HIGH : LOW);
  }
}
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/wss");
void initWebServer()
{
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.begin();
}
// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------
void notifyClients(bool state) {
    ws.textAll(state ? "on" : "off");
    //ws.binaryAll()
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        if (strcmp((char*)data, "toggle") == 0) {
            bool state=digitalRead(LED_BUILTIN);
            digitalWrite(LED_BUILTIN, !state);
            notifyClients(state);
        }
    }
}
void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,
             void                 *arg,
             uint8_t              *data,
             size_t                len) {

    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  dac_output_enable(DAC_CHANNEL_1);
  delay(500);
  initSPIFFS();
  initWiFi();
  initWebSocket();
  initWebServer();
}

void loop()
{
  digitalWrite(LED_BUILTIN, 1);
  ws.textAll("LED_ON");
  delay(2000);
  digitalWrite(LED_BUILTIN, 0);
  delay(1000);
  ws.textAll("LED_OFF");
}