#include "asterisk.h"
#include <asterisk/module.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <stddef.h> // size_t 정의를 위한 헤더 파일 추가

// C++ 함수 선언
void websocket_client_connect();
void websocket_client_send(const char *data, size_t len, bool is_rx);
void websocket_client_close();

static int init_websocket(void) {
    websocket_client_connect();
    return AST_MODULE_LOAD_SUCCESS;
}

static int send_to_websocket(const char *data, size_t len, bool is_rx) {
    websocket_client_send(data, len, is_rx);
    return 0;
}

static int unload_module(void) {
    websocket_client_close();
    return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "WebSocket Module",
    .load = init_websocket,
    .unload = unload_module,
);
