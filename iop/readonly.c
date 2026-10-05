/* Original PSXCore code, Danny Nunez (dnunezx), 2026.
 * Installed before ATA/APA. Deny block writes, including journal repair.
 * Deliberately declines filesystem ownership so APA can continue mounting.
 */
#ifdef NP_HOST_TEST
#include "readonly_stub.h"
#else
#include <bdm.h>
#include <irx.h>
#include <loadcore.h>
#endif
#include <errno.h>
#include <stdio.h>
IRX_ID("psxcore_ro", 1, 0);

static int deny_write(struct block_device *bd, u64 sector,
                      const void *buffer, u16 count) {
    (void)bd; (void)sector; (void)buffer; (void)count;
    printf("PSXCORE write-blocked\n");
    return -EROFS;
}
static int protect(struct block_device *bd) {
    if (bd && bd->write != deny_write) {
        bd->write = deny_write;
        printf("PSXCORE readonly-guard=attached device=%s%up%u\n",
               bd->name, bd->devNr, bd->parNr);
    }
    return -1;
}
static void disconnect(struct block_device *bd) { (void)bd; }
static struct file_system guard = {
    .priv = NULL, .name = "psxcore-ro",
    .connect_bd = protect, .disconnect_bd = disconnect,
};
int _start(int argc, char **argv) {
    (void)argc; (void)argv;
    bdm_connect_fs(&guard);
    return MODULE_RESIDENT_END;
}
