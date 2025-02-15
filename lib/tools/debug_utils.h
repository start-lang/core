#include <start_lang.h>

#ifndef LANG_TOOLS_H
#define LANG_TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

uint8_t debug_state(State * s, uint8_t force_debug, uint8_t style);

void print_exec_info(void);

#ifdef __cplusplus
}
#endif

#endif