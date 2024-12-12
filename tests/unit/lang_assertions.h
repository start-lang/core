#include <stdint.h>

#ifndef START_LANG_VALIDATOR_H
#define START_LANG_VALIDATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ENABLE_FILES
char * load_file(const char * fname);
#endif

int8_t run(char * src);
char * getBuffer(void);
int clean(void);
void print_info(void);
void validate(void);

#ifdef __cplusplus
}
#endif

#endif
