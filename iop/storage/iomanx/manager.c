/* Keep the SDK's separate iomanx library and descriptor tables without its
 * ROM-IOMAN takeover. POPS IOPRP252 uses linked device entries and 32 files;
 * the SDK hook assumes a 16-pointer device array followed by 16 files.
 * Importing that layout corrupts this registry and redirects native callers.
 * POPStarter's USB route leaves native IOMAN in charge of native devices.
 * PSXCore's FAT/exFAT devices belong only to this private SDK registry. */
#include "iomanX.c"

int hook_ioman(void) { return 0; }
int unhook_ioman(void) { return 0; }
