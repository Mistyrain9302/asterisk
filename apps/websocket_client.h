#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <cstddef> // size_t 정의를 위한 헤더 파일 추가

extern "C" {
    void websocket_client_connect(void);
    void websocket_client_send(const char *data, size_t len, int is_rx);
    void websocket_client_close(void);
}

#endif // WEBSOCKET_CLIENT_H
