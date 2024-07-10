#include <Arduino.h>
#include <HsHWebsocket/HsHWebsocket.h>

#define SERVER_URL "https://echo.websocket.org/"

void setup() {
  Serial.begin(115200);

  webSocket.onConnect([]() {
    Serial.println("Connected");
  });

  webSocket.onDisconnect([]() {
    Serial.println("Disconnected");
  });

  webSocket.onFrame([](char* data, size_t length) {
    Serial.println(data);
  });

  webSocket.onError([](WsError error) {
    Serial.println(error.message);
  });

  webSocket.listen(SERVER_URL);
}

void loop() {
  
}