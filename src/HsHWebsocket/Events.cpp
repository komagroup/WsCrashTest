#include "HsHWebsocket.h"

void HsHWebsocket::onConnect(Callback callback) {
    connectCallback = callback;
}

void HsHWebsocket::onDisconnect(Callback callback) {
    disconnectCallback = callback;
}

void HsHWebsocket::onFrame(FrameCallback callback) {
    frameCallback = callback;
}

void HsHWebsocket::onBinaryFrame(BinaryFrameCallback callback) {
    binaryFrameCallback = callback;
}

void HsHWebsocket::onError(ErrorCallback callback) {
    errorCallback = callback;
}

void HsHWebsocket::onPing(Callback callback){
    pingCallback = callback;
}
void HsHWebsocket::onPong(Callback callback){
    pongCallback = callback;
}

void HsHWebsocket::invokeCallback(Callback callback) {
    if (callback) { callback(); }
}

void HsHWebsocket::invokeErrorCallback(WSErrorCode error) {
    if (errorCallback) { errorCallback( createError(error) ); }
}