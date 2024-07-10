#include "HsHWebsocket.h"

void HsHWebsocket::disconnect(){
    if( connected && !isConnecting ){
        printf("STOPPING!!");
        client.stop();
        connected = false;
        invokeCallback(disconnectCallback);
    }
}

void HsHWebsocket::softDisconnect(){
    connected = false;
}