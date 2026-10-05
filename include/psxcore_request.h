/* Original PSXCore dry-run contract, 2026. No runtime ABI is implied. */
#ifndef PSXCORE_REQUEST_H
#define PSXCORE_REQUEST_H
#include <stddef.h>
#include <stdint.h>
#define NP_REQUEST_MAGIC UINT32_C(0x5244504e)
#define NP_REQUEST_VERSION 1
#define NP_REQUEST_DRY_RUN 1
/* Fixed-size, pointer-free fields. Integer fields are little-endian on PS2. */
typedef struct {
    uint32_t magic;
    uint16_t version, size;
    uint8_t platform, backend;
    uint16_t flags;
    uint32_t reserved;
    char device[8], partition[64], path[512], identity[640];
} NpRequest;
_Static_assert(sizeof(NpRequest) == 1240, "request layout changed");
const char *np_request_build(NpRequest *out, const char *device,
                             const char *partition, const char *path);
const char *np_request_check(const void *bytes, size_t length);
#endif
