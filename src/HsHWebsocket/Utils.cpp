#include "HsHWebsocket.h"

void HsHWebsocket::addHeader(const char* key, const char* value) {
    String header;
    header += key;
    header += ": ";
    header += value;
    headers.push_back(header.c_str());
}

void HsHWebsocket::generateWebSocketKey(char* keyBuffer, size_t length) {
    esp_fill_random((unsigned char*)keyBuffer, length);
}

void HsHWebsocket::base64_encode(const unsigned char* input, size_t length, char* output) {
    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                *output++ = base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            *output++ = base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            *output++ = '=';
        }
    }

    *output = '\0';
}

void HsHWebsocket::extractHostname(const char* url) {
    const char* start = strstr(url, "://");
    if (start) {
        start += 3;  // Skip past "://"
    } else {
        start = url;  // No protocol found, use the whole URL
    }

    // Find the end of the hostname (start of the path or end of string)
    const char* end = strchr(start, '/');
    if (!end) {
        end = url + strlen(url);
    }

    // Copy the hostname
    strncpy(hostname, start, end - start);
    hostname[end - start] = '\0';
}

int HsHWebsocket::extractPort(const char* url) {
    const char* colon = strchr(url, ':');
    if (colon) {
        return atoi(colon + 1);
    } else {
        return 80;  // Default port
    }
}

int HsHWebsocket::determinePort(const char* url) {
    const char* https = strstr(url, "https");
    const char* wss = strstr(url, "wss");
    if (https || wss) { return 443; }
    return 80;
}

void HsHWebsocket::setClient() {
    if (port == 443) {
        secureClient.setInsecure();
        client = (WiFiClient&)secureClient;
        return;
    }
    client = normalClient;
}

bool HsHWebsocket::isConnected() {
    return /*client.connected() && */connected;
}

WsError HsHWebsocket::createError(WSErrorCode code) {
    WsError error;
    error.code = code;
    switch (code) {
        case WS_HANDSHAKE:
            error.message = "Failed to perform handshake";
            break;
        case WS_CONN_CLOSE_FRAME:
            error.message = "Got a connection close frame from the server.";
            break;
        case WS_UNKNOWN_OP_CODE:
            error.message = "Received an unknown OP code. Turn on debug for more info.";
            break;
        case WS_EMPTY_PAYLOAD_DATA:
            error.message = "Received empty payload data from the server.";
            break;
        case WS_INVALID_MASKING_FRAME:
            error.message = "Got an invalid masking frame from the server.";
            break;
        case WS_EMPTY_FRAME_HEADER:
            error.message = "Empty frame header.";
            break;
        case WS_MALFORMED_EXTENDED_PAYLOAD_126:
            error.message = "Malformed extended payload (126).";
            break;
        case WS_MALFORMED_EXTENDED_PAYLOAD_127:
            error.message = "Malformed extended payload (127).";
            break;
        default:
            error.message = "Unknown error";
            break;
    }
    return error;
}