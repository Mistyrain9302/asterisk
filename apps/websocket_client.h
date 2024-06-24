#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cstddef> // 또는 <stddef.h>

extern "C" {
    void websocket_client_connect();
    void websocket_client_send(const char* data, size_t len, bool is_rx);
    void websocket_client_close();
}

#endif // WEBSOCKET_CLIENT_H
