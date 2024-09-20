#ifndef AST_MODULE_SELF_SYM
#define AST_MODULE_SELF_SYM __internal_self
#endif

#include "asterisk.h"
#include <asterisk/module.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <stddef.h> // size_t 정의를 위한 헤더 파일 추가
#include "websocket_client.h" 

#if !defined(AST_MODULE_INFO)

#define AST_MODULE_INFO(keystr, flags_to_set, desc, fields...)	\
	static struct ast_module_info 				\
		__mod_info = {					\
		.name = AST_MODULE,				\
		.flags = flags_to_set,				\
		.description = desc,				\
		.key = keystr,					\
		.buildopt_sum = AST_BUILDOPT_SUM,		\
		fields						\
	};							\
	static void  __attribute__((constructor)) __reg_module(void) \
	{ \
		ast_module_register(&__mod_info); \
	} \
	static void  __attribute__((destructor)) __unreg_module(void) \
	{ \
		ast_module_unregister(&__mod_info); \
	} \
	struct ast_module *AST_MODULE_SELF_SYM(void)                       \
	{                                                                  \
		return __mod_info.self;                                        \
	}                                                                  \
	static const struct ast_module_info *ast_module_info = &__mod_info
#endif



static int load_module(void) {
    websocket_client_connect();
    return AST_MODULE_LOAD_SUCCESS;
}

static int unload_module(void) {
    websocket_client_close();
    return AST_MODULE_LOAD_SUCCESS;
}

int send_to_websocket(const char *data, size_t len, int is_rx) {
    websocket_client_send(data, len, is_rx);
    return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "WebSocket Module",
    .load = load_module,
    .unload = unload_module,
    .reload = NULL,
    .load_pri = 0,
    .support_level = AST_MODULE_SUPPORT_CORE
);
