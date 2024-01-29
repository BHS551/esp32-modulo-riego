#include <Arduino.h>
#include <WifiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "HEREDIA"
#define WIFI_PASS "3208035673"

#define WS_HOST ""
#define WS_PORT 443
#define WS_URL ""

#define JSON_DOC_SIZE 2048
#define MSG_SIZE 256

WiFiMulti wifiMulti;
WebSocketsClient wsClient;

void sendErrorMessage(const char *error)
{
  char msg[MSG_SIZE];

  sprintf(msg, "{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}",
          error);

  wsClient.sendTXT(msg);
}

void sendOkMessage()
{
  wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"ok\"}");
}

void handleMessage(uint8_t *payload)
{
  StaticJsonDocument<JSON_DOC_SIZE> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    sendErrorMessage(error.c_str());
    return;
  }

  if (!doc["type"].is<const char *>())
  {
    sendErrorMessage("invalid message type format");
    return;
  }

  if (strcmp(doc["type"], "cmd") == 0)
  {
    if (!doc["body"].is<JsonObject>())
    {
      sendErrorMessage("invalid command body");
      return;
    }

    if (strcmp(doc["body"]["type"], "pinMode") == 0)
    {
      /*
      Uncomment this code for better validation of pinMode command

      if (!doc["body"]["mode"].is<const char *>()) {
        sendErrorMessage("invalid pinMode mode type");
        return;
      }

      if (strcmp(doc["body"]["mode"], "input") != 0 &&
          strcmp(doc["body"]["mode"], "input_pullup") != 0 &&
          strcmp(doc["body"]["mode"], "output") != 0) {
        sendErrorMessage("invalid pinMode mode value");
        return;
      }
      */

      pinMode(doc["body"]["pin"], toMode(doc["body"]["mode"]));
      sendOkMessage();
      return;
    }

    if (strcmp(doc["body"]["type"], "digitalWrite") == 0)
    {
      digitalWrite(doc["body"]["pin"], doc["body"]["value"]);
      sendOkMessage();
      return;
    }

    if (strcmp(doc["body"]["type"], "digitalRead") == 0)
    {
      auto value = digitalRead(doc["body"]["pin"]);

      char msg[MSG_SIZE];

      sprintf(msg, "{\"action\":\"msg\",\"type\":\"output\",\"body\":%d}",
              value);

      wsClient.sendTXT(msg);
      return;
    }

    sendErrorMessage("unsupported command type");
    return;
  }

  sendErrorMessage("unsupported message type");
  return;
}

void onEvent(WStype type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("WS Connected");
    break;
  case WStype_DISCONNECTED:
    Serial.println("WS Disconnected");
    break;
  case WStype_TEXT:
    Serial.println("WS Message: %s\n", payload);
    break;
  }
}

void awaitWifiConnection()
{
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(100);
  }
}

void connectWebSocket()
{
  wsClient.beginSSL(WS_HOST, WS_PORT, WS_URL, "", "wss");
  wsClient.onEvent(WS_HOST, WS_PORT, WS_URL, "", "wss");
}

void setup()
{
  Serial.begin(921600);
  Serial.println("setting up...");

  pinMode(LED_BUILTIN, OUTPUT);

  awaitWifiConnection();
  connectWebSocket();
  Serial.println("connected");
}

void loop()
{
  delay(1000);
  Serial.println("looping...");
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED);
  delay(1000);
}
