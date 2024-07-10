#include "HsHWebsocket.h"

HsHWebsocket::HsHWebsocket(){
    sendMutex = xSemaphoreCreateMutex();
    connected = false;
    receiveBuffer = (char*)malloc(RECEIVE_BUFFER_SIZE);
    sendBuffer = (uint8_t*)malloc(MAX_FRAME_SIZE);
}

HsHWebsocket::~HsHWebsocket() {
    disconnect();
    if (sendMutex) { vSemaphoreDelete(sendMutex); }
    if (receiveBuffer) { free(receiveBuffer); }
    if (sendBuffer) { free(sendBuffer); }
}

void HsHWebsocket::listen(const char* _url, const char* _path) {
    if( isInited() ){
        printf("[WS] - Already inited\n");
        return;
    }
    // copy _url to url
    strncpy(url, _url, sizeof(url) - 1);
    url[sizeof(url) - 1] = '\0';

    // copy _path to path
    strncpy(path, _path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    // set port
    port = determinePort(_url);

    useIp = false;
    start();
}

void HsHWebsocket::listen(IPAddress _ip, uint16_t _port, const char* _path) {
    if( isInited() ){
        printf("[WS] - Already inited\n");
        return;
    }
    // Set the ip and the port
    this->ip = _ip;
    this->port = _port;
    
    // copy _path to path
    strncpy(path, _path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    useIp = true;
    start();
}

void HsHWebsocket::start() {
    initialised = true;
    setClient();
    extractHostname(url);
    makeTask();
}

bool HsHWebsocket::isInited(){
    return initialised;
}