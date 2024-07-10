#include "HsHWebsocket.h"

void HsHWebsocket::sendPing() {
    if (!isConnected()) return;
    if (xSemaphoreTake(sendMutex, portMAX_DELAY) == pdTRUE) {
        // Ping frame format
        size_t frameSize = 6;  // 2 bytes for header + 4 bytes for mask key (no payload)

        char frame[frameSize];
        frame[0] = 0x89;         // FIN + Ping frame opcode
        frame[1] = 0x80 | 0x00;  // Masked with payload length 0

        // Generate a masking key
        uint8_t maskKey[4];
        esp_fill_random(maskKey, 4);
        memcpy(frame + 2, maskKey, 4);

        client.write((const uint8_t*)frame, frameSize);
        xSemaphoreGive(sendMutex);
    }
}

void HsHWebsocket::sendPong() {
    if (!isConnected()) return;
    if (xSemaphoreTake(sendMutex, portMAX_DELAY) == pdTRUE) {
        // Pong frame format
        size_t frameSize = 6;  // 2 bytes for header + 4 bytes for mask key (no payload)

        char frame[frameSize];
        frame[0] = 0x8A;         // FIN + Pong frame opcode
        frame[1] = 0x80 | 0x00;  // Masked with payload length 0

        // Generate a masking key
        uint8_t maskKey[4];
        esp_fill_random(maskKey, 4);
        memcpy(frame + 2, maskKey, 4);

        client.write((const uint8_t*)frame, frameSize);
        xSemaphoreGive(sendMutex);
    }
}

void HsHWebsocket::createHeader(uint8_t headerSize, int messageLength){
    // Create header
    sendBuffer[0] = FIN | OPCODE_TEXT;
    if (messageLength < 126) {
        sendBuffer[1] = 0x80 | messageLength;
    } else if (messageLength < 0xFFFF) {
        sendBuffer[1] = 0x80 | 126;
        sendBuffer[2] = (messageLength >> 8) & 0xFF;
        sendBuffer[3] = messageLength & 0xFF;
    } else {
        sendBuffer[1] = 0x80 | 127;
        sendBuffer[2] = (messageLength >> 56) & 0xFF;
        sendBuffer[3] = (messageLength >> 48) & 0xFF;
        sendBuffer[4] = (messageLength >> 40) & 0xFF;
        sendBuffer[5] = (messageLength >> 32) & 0xFF;
        sendBuffer[6] = (messageLength >> 24) & 0xFF;
        sendBuffer[7] = (messageLength >> 16) & 0xFF;
        sendBuffer[8] = (messageLength >> 8) & 0xFF;
        sendBuffer[9] = messageLength & 0xFF;
    }
    memcpy(sendBuffer + headerSize - 4, maskKey, 4);
}

void HsHWebsocket::send(const char* message, int messageLength) {
    if (!isConnected() || !message || messageLength <= 0) return;

    uint8_t headerSize = messageLength < 126? 2 : messageLength < 0xFFFF? 4 : 10;
    // Add mask key size if client
    headerSize += 4;
    // Generate mask key
    for (uint8_t i = 0; i < 4; ++i) {
        maskKey[i] = random(0x00, 0xFF);
    }
    
    // Create header
    createHeader(headerSize, messageLength);

    // Write the header
    if (client.write(sendBuffer, headerSize) != headerSize) {
        #if WS_DEBUG
            printf("[WS] - Failed to send header\n");
        #endif
        softDisconnect();
        return;
    }

    // Send payload in chunks if necessary
    size_t chunkSize = MAX_FRAME_SIZE - headerSize;
    size_t offset = 0;

    while (offset < messageLength) {
        size_t bytesToSend = min(chunkSize, messageLength - offset);

        // Mask the payload in chunks and send
        for (size_t i = 0; i < bytesToSend; ++i) {
            sendBuffer[headerSize + i] = message[offset + i] ^ maskKey[(offset + i) % 4];
        }

        if (client.write(sendBuffer + headerSize, bytesToSend) != bytesToSend) {
            log_e("Failed to send payload chunk\n");
            return;
        }

        offset += bytesToSend;
    }

    client.flush();
}

void HsHWebsocket::sendText(const char* message, int messageLength) {
    if (!isConnected() || !message || messageLength <= 0) return;
    if (xSemaphoreTake(sendMutex, portMAX_DELAY) == pdTRUE) {
        send(message, messageLength);
        xSemaphoreGive(sendMutex);
    }
}

void HsHWebsocket::sendText(const char* message) {
    if (!isConnected() || !message) return;
    sendText(message, strlen(message));
}