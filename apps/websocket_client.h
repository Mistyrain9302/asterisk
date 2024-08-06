#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <stddef.h> // size_t 정의를 위한 헤더 파일 추가
#include <string>

extern "C" {
    void websocket_client_connect(void);
    void websocket_client_send(const char *data, size_t len, int is_rx);
    void websocket_client_close(void);
    void websocket_client_send_json(const std::string& json_data, int is_rx);
}

#endif // WEBSOCKET_CLIENT_H
