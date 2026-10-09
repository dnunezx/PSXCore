/* Private resident bridge to the original POPS IOP driver registry. */
#ifndef PSXCORE_NATIVEGATE_H
#define PSXCORE_NATIVEGATE_H
typedef struct NpNativeDevice NpNativeDevice;
int np_file_native_adddrv(NpNativeDevice *device);
#endif
