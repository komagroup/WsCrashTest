#include "HsHWebsocket.h"

void HsHWebsocket::makeTask() {
    xTaskCreate([](void* param) {
        static_cast<HsHWebsocket*>(param)->connect();
        while (true){
            static_cast<HsHWebsocket*>(param)->handle();
            vTaskDelay(1);
        }
        vTaskDelete(nullptr);
    },"HsHWsTask", WS_TASK_STACK, this, WS_TASK_PRIORITY, &taskHandle);
}

void HsHWebsocket::connect() {
    if(isConnecting || isConnected()){ return; }
    #if WS_DEBUG
        printf("[WS] - Connecting to %s\n", url);
        printf("[WS] - Hostname: %s\n", hostname);
        printf("[WS] - Path: %s\n", path);
        printf("[WS] - Port: %d\n", port);
        printf("[WS] - Reconnect interval: %dms\n", reconnectInterval);
    #endif
    
    isConnecting = true;
    while (!client.connect(hostname, port)) {
        vTaskDelay(reconnectInterval);
        #if WS_DEBUG
            printf("[WS] - Couldn't connect. Reconnecting...\n");
        #endif
    }
    handshake();
    waitResponse();
    if( !isConnected() ){
        invokeErrorCallback(WS_HANDSHAKE);
    }else{
        invokeCallback(connectCallback);
    }
    isConnecting = false;
}

void HsHWebsocket::handshake() {
    // Create the Sec-WebSocket-Key
    char websocketKeyBase64[25];
    uint8_t websocketKey[16];
    generateWebSocketKey((char*)websocketKey, sizeof(websocketKey));
    base64_encode((unsigned char*)websocketKey, sizeof(websocketKey), websocketKeyBase64);

    client.printf("GET /%s HTTP/1.1\r\n", path);
    if( !useIp ){
        client.printf("Host: %s:%d\r\n", hostname, port);
    }else{
        client.printf("Host: %s:%d\r\n", ip.toString().c_str(), port);
    }
    client.printf("Upgrade: websocket\r\n");
    client.printf("Connection: Upgrade\r\n");
    client.printf("Sec-WebSocket-Key: %s\r\n", websocketKeyBase64);
    client.printf("Sec-WebSocket-Version: 13\r\n");

    // Add the headers
    for (const auto& header : headers) {
        if (header.length() == 0) continue;
        client.printf("%s\r\n", header.c_str());
    }

    client.printf("\r\n");
    client.flush();
}

void HsHWebsocket::waitResponse(){
    // Reading the complete response, including any additional data after newline
    String response;
    while (client.connected()) {
        if(client.available()){
            String line = client.readStringUntil('\n');
            line.trim(); // Remove any trailing whitespace including \r
            log_i("Received line: %s", line.c_str());
            response += line + "\n";
            if (line == "") { break; }
        }
    }
    connected = response.indexOf("101 Switching Protocols") >= 0;
    // Clear buffer after handshake to avoid mixing header data with frame data
    while (client.available()) {
        // Discard any lingering data in the buffer
        client.read();
    }
}