/* Production startup stays silent until a checked launch failure. */
#include "startup_diag.h"
#include <stdio.h>
#ifndef NP_HOST_TEST
#include <debug.h>
#endif
static char first_error[137];
void np_startup_error(const char *code) {
    if(!first_error[0] && code)snprintf(first_error,sizeof(first_error),"%s",code);
}
void np_startup_stop(const char *fallback) {
    np_startup_error(fallback);
#ifndef NP_HOST_TEST
    init_scr();scr_setCursor(0);
    scr_printf("PSXCore could not start the game.\nError: %s\n",first_error);
#endif
}
