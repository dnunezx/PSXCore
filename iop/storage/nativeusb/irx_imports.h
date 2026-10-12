#include <irx.h>
#include <loadcore.h>
#include <iomanX.h>
#include <sysclib.h>
#include <stdio.h>
#include "../../filebridge/native.h"
int np_mass_open(const char *,int,int);
int np_mass_close(int);
int np_mass_read(int,void *,int);
int np_mass_write(int,void *,int);
int np_mass_seek(int,int,int);
int np_mass_remove(const char *);
int np_mass_dopen(const char *);
int np_mass_dclose(int);
int np_mass_stat(const char *,NpNativeStat *);
int np_mass_rename(const char *,const char *);
int np_mass_ioctl2(int,int,void *,unsigned,void *,unsigned);
/* Native IOMAN 1.4 uses these extended signatures and export ordinals.
 * The bootstrap verifies its firmware and nativegate revision before load. */
#define native_mass_IMPORTS_start DECLARE_IMPORT_TABLE(ioman,1,4)
#define native_mass_IMPORTS_end END_IMPORT_TABLE
