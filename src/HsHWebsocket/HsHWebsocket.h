#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>


#define WS_DEBUG 0
#define MAX_HEADERS 10
#define MAX_PAYLOAD_SIZE 1024
#define MAX_FRAME_SIZE MAX_PAYLOAD_SIZE + 14
#define OPCODE_TEXT 0x01
#define FIN 0x80

#define RECEIVE_BUFFER_SIZE 100000
#define WS_TASK_STACK 5000
#define WS_TASK_PRIORITY 5

typedef enum WSErrorCode {
    WS_HANDSHAKE = 0,
    WS_CONN_CLOSE_FRAME = 1,
    WS_UNKNOWN_OP_CODE = 2,
    WS_EMPTY_PAYLOAD_DATA = 3,
    WS_INVALID_MASKING_FRAME = 4,
    WS_EMPTY_FRAME_HEADER = 5,
    WS_MALFORMED_EXTENDED_PAYLOAD_126 = 6,
    WS_MALFORMED_EXTENDED_PAYLOAD_127 = 7
}WSErrorCode;

struct WsError {
    WSErrorCode code;
    const char* message;
};

class HsHWebsocket {
   public:
    using Callback = std::function<void()>;
    using FrameCallback = std::function<void(char*, size_t)>;
    using ErrorCallback = std::function<void(WsError)>;
    using BinaryFrameCallback = std::function<void(const uint8_t*, size_t)>;

    HsHWebsocket();
    ~HsHWebsocket();

    bool isInited();

    void listen(const char* url, const char* path = "/");
    void listen(IPAddress ip, uint16_t port, const char* path = "/");

    /* Events */
    void onConnect(Callback callback);
    void onDisconnect(Callback callback);
    void onPing(Callback callback);
    void onPong(Callback callback);
    void onError(ErrorCallback callback);
    void onFrame(FrameCallback callback);
    void onBinaryFrame(BinaryFrameCallback callback);

    /* Utils */
    void addHeader(const char* key, const char* value);
    bool isConnected();

    /* Send */
    void sendText(const char* message, int messageLength);
    void sendText(const char* message);

   private:
    std::vector<std::string> headers;

    char url[256];
    char path[256];
    char hostname[256];

    IPAddress ip;
    uint16_t port;
    uint16_t reconnectInterval = 5000;

    WiFiClientSecure secureClient;
    WiFiClient normalClient;
    WiFiClient& client = secureClient;
    TaskHandle_t taskHandle;

    bool useIp;
    bool connected;
    bool fragmentedMessage;
    bool isConnecting = false;
    bool initialised = false;

    Callback connectCallback;
    Callback disconnectCallback;
    Callback pingCallback;
    Callback pongCallback;
    ErrorCallback errorCallback;
    FrameCallback frameCallback;
    BinaryFrameCallback binaryFrameCallback;

    size_t accumulatedLength;
    uint8_t maskKey[4];
    uint8_t* sendBuffer;
    char* receiveBuffer;

    /* Connect */
    void start();
    void makeTask();
    void connect();
    void handshake();
    void waitResponse();

    /* Disconnect */
    void disconnect();
    void softDisconnect();

    /* Send */
    SemaphoreHandle_t sendMutex;
    void send(const char* message, int messageLength);
    void sendPing();
    void sendPong();
    void createHeader(uint8_t headerSize, int messageLength);

    /* Handle */
    void handle();
    void processFrame(uint8_t opcode);
    bool readFrameHeader(uint8_t& opcode, uint64_t& payloadLength, bool& fin, bool& masked);
    bool readMaskingKey(uint8_t* maskKey);
    bool readPayloadData(uint64_t payloadLength, bool masked, uint8_t* maskKey);

    /* Utils */
    void generateWebSocketKey(char* keyBuffer, size_t length);
    void base64_encode(const unsigned char* input, size_t length, char* output);
    void setClient();
    void extractHostname(const char* url);

    int extractPort(const char* url);
    int determinePort(const char* url);

    WsError createError(WSErrorCode code);

    /* Events */
    void invokeCallback(Callback callback);
    void invokeErrorCallback(WSErrorCode error);
};

inline HsHWebsocket webSocket;