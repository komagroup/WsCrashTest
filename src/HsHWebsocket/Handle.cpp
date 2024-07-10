#include "HsHWebsocket.h"

void HsHWebsocket::handle() {
    if (!isConnected()) {
        disconnect();
        connect();
        return;
    }

    if (!client.available()) { return; }

    uint8_t opcode;
    uint64_t payloadLength;
    bool fin, masked;

    if (!readFrameHeader(opcode, payloadLength, fin, masked)) {
        return;
    }

    if (masked && !fragmentedMessage) {
        if (!readMaskingKey(maskKey)) {
            return;
        }
    }

    if (!readPayloadData(payloadLength, masked, maskKey)) {
        return;
    }

    if (fin) {
        processFrame(opcode);
    } else {
        fragmentedMessage = true;
    }
}

bool HsHWebsocket::readFrameHeader(uint8_t &opcode, uint64_t &payloadLength, bool &fin, bool &masked) {
    int length = client.readBytes(receiveBuffer, 2);
    if (length != 2) {
        invokeErrorCallback(WS_EMPTY_FRAME_HEADER);
        return false;
    }

    fin = receiveBuffer[0] & 0x80;
    opcode = receiveBuffer[0] & 0x0F;
    masked = receiveBuffer[1] & 0x80;
    payloadLength = receiveBuffer[1] & 0x7F;

    if (payloadLength == 126) {
        length = client.readBytes(receiveBuffer, 2);
        if (length != 2) {
            #if WS_DEBUG
                printf("[WS] - Failed to read extended payload length (126)\n");
            #endif
            invokeErrorCallback(WS_MALFORMED_EXTENDED_PAYLOAD_126);
            return false;
        }
        payloadLength = (receiveBuffer[0] << 8) | receiveBuffer[1];
    } else if (payloadLength == 127) {
        length = client.readBytes(receiveBuffer, 8);
        if (length != 8) {
            #if WS_DEBUG
                printf("[WS] - Failed to read extended payload length (127)\n");
            #endif
            invokeErrorCallback(WS_MALFORMED_EXTENDED_PAYLOAD_127);
            return false;
        }
        payloadLength = 0;
        for (int i = 0; i < 8; i++) {
            payloadLength = (payloadLength << 8) | receiveBuffer[i];
        }
    }

    return true;
}

bool HsHWebsocket::readMaskingKey(uint8_t *maskKey) {
    int length = client.readBytes((char *)maskKey, 4);
    if (length != 4) {
        #if WS_DEBUG
            printf("[WS] - Failed to read masking key\n");
        #endif
        invokeErrorCallback(WS_INVALID_MASKING_FRAME);
        return false;
    }
    return true;
}

bool HsHWebsocket::readPayloadData(uint64_t payloadLength, bool masked, uint8_t *maskKey) {
    if (accumulatedLength + payloadLength > RECEIVE_BUFFER_SIZE) {
        #if WS_DEBUG
            printf("[WS] - Buffer overflow, dropping frame\n");
        #endif
        accumulatedLength = 0;
        fragmentedMessage = false;
        return false;
    }

    size_t bytesRead = 0;
    while (bytesRead < payloadLength) {
        int length = client.readBytes(receiveBuffer + accumulatedLength + bytesRead, payloadLength - bytesRead);
        if (length <= 0) {
            invokeErrorCallback(WS_EMPTY_PAYLOAD_DATA);
            return false;
        }
        bytesRead += length;
    }

    if (masked) {
        for (uint64_t i = 0; i < payloadLength; i++) {
            receiveBuffer[accumulatedLength + i] ^= maskKey[i % 4];
        }
    }

    accumulatedLength += payloadLength;
    return true;
}

void HsHWebsocket::processFrame(uint8_t opcode) {
    receiveBuffer[accumulatedLength] = '\0';

    switch (opcode) {
        case 0x01:  // Text frame
            if (frameCallback) {
                frameCallback(receiveBuffer, accumulatedLength);
            }
            break;
        case 0x02:  // Binary frame
            if (binaryFrameCallback) {
                binaryFrameCallback((const uint8_t *)receiveBuffer, accumulatedLength);
            }
            break;
        case 0x08:  // Connection close
            #if WS_DEBUG
                printf("[WS] - Received connection close frame.\n");
            #endif
            softDisconnect();
            invokeErrorCallback(WS_CONN_CLOSE_FRAME);
            break;
        case 0x09:  // Ping
            #if WS_DEBUG
                printf("Received ping frame.\n");
            #endif
            if (pingCallback) {
                pingCallback();
            }
            sendPong();
            break;
        case 0x0A:  // Pong
            #if WS_DEBUG
                printf("[WS] - Received pong frame.\n");
            #endif
            if (pongCallback) {
                pongCallback();
            }
            break;
        default:
            #if WS_DEBUG
                printf("[WS] - Received unknown frame: opcode=%d\n", opcode);
            #endif
            invokeErrorCallback(WS_UNKNOWN_OP_CODE);
            break;
    }

    accumulatedLength = 0;
    fragmentedMessage = false;
}