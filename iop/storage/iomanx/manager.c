/* Keep the SDK's separate iomanx library and descriptor tables without its
 * ROM-IOMAN takeover. POPS IOPRP252 uses linked device entries and 32 files;
 * the SDK hook assumes a 16-pointer device array followed by 16 files.
 * Importing that layout corrupts this registry and redirects native callers.
 * POPStarter's USB route leaves native IOMAN in charge of native devices.
 * PSXCore's FAT/exFAT devices belong only to this private SDK registry. */
#include "iomanX.c"

/* A private registry still needs the SDK's standard descriptor reservation.
 * Otherwise the first disc/card opens reuse stdin/stdout (0/1). Register only
 * the SDK's local tty; the native IOMAN registry remains untouched. */
int hook_ioman(void) {
    register_tty();
    if(iomanX_open("tty:",FIO_O_RDONLY,0)!=0)return -1;
    if(iomanX_open("tty:",FIO_O_WRONLY,0)!=1) {
        iomanX_close(0);return -1;
    }
    return 0;
}
int unhook_ioman(void) { return 0; }
