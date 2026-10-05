/* Original bounded parser for the bootstrap's exact, newline-terminated modes. */
#ifndef PSXCORE_RUNTIME_CONFIG_H
#define PSXCORE_RUNTIME_CONFIG_H
#include <stddef.h>
#include <string.h>
static int np_runtime_mode_valid(const void *bytes, size_t length) {
    static const char *const modes[] = {
        "stock\n", "image\n", "hdd\n", "hddtrace\n", "hdddisc\n", "hddgame\n",
        "hddadapt\n", "hddverify\n", "hddlicense\n", "hddlicensequiet\n",
        "hddlicenseassets\n", "hddlicensegraphics\n", "hddlicensegraphicsfix\n",
        "hddlicensereadback\n", "hddlicensetekken\n", "launch\n"
    };
    if (!bytes) return 0;
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
        if (length == strlen(modes[i]) && !memcmp(bytes, modes[i], length)) return 1;
    return 0;
}
#endif
