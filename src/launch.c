#include "psxcore_launch.h"
#include <stdio.h>
#include <string.h>

static int partition_valid(const char *s) {
    size_t n = strlen(s);
    if (strncmp(s, "hdd0:", 5) || n <= 5 || n > 37) return 0;
    for (const unsigned char *p = (const unsigned char *)s + 5; *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
            (*p >= '0' && *p <= '9') || *p == '.' || *p == '_' || *p == '-')) return 0;
    return 1;
}
static int nonzero(const unsigned char *p, size_t n) {
    unsigned any = 0;
    while (n--) any |= *p++;
    return any != 0;
}
void np_launch_init(NpLaunchRequest *r) {
    if (!r) return;
    memset(r, 0, sizeof(*r));
    r->magic = NP_LAUNCH_MAGIC; r->version = NP_LAUNCH_VERSION; r->size = sizeof(*r);
    r->runtime = NP_RUNTIME_KNOWN;
    r->options.diagnostics = 1;
}
const char *np_launch_check(const void *bytes, size_t length, NpCompatResult *resolved) {
    NpLaunchRequest r;
    NpCompatResult temporary;
    if (resolved) memset(resolved, 0, sizeof(*resolved));
    if (!bytes || length != sizeof(r)) return "launch-size";
    memcpy(&r, bytes, sizeof(r));
    if (r.magic != NP_LAUNCH_MAGIC || r.version != NP_LAUNCH_VERSION || r.size != sizeof(r))
        return "launch-version";
    if (r.reserved || nonzero(r.padding, sizeof(r.padding)) || r.save_mode > NP_SAVE_SHARED)
        return "launch-flags";
    if (!memchr(r.disc_partition, 0, sizeof(r.disc_partition)) ||
        !memchr(r.save_partition, 0, sizeof(r.save_partition)) ||
        !memchr(r.save_key, 0, sizeof(r.save_key)) || !memchr(r.serial, 0, sizeof(r.serial)))
        return "launch-string";
    if (!partition_valid(r.disc_partition) || !partition_valid(r.save_partition)) return "launch-partition";
    /* Native argument parser requires a nonempty suffix after the first dot. */
    const char *dot = strchr(r.disc_partition + 5, '.');
    if (!dot || !dot[1]) return "launch-native-suffix";
    size_t keylen = strlen(r.save_key);
    if (!keylen || keylen > 20) return "launch-save-key";
    for (size_t i = 0; i < keylen; ++i) {
        unsigned char c = (unsigned char)r.save_key[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-')) return "launch-save-key";
    }
    char expected[40];
    memcpy(expected, "hdd0:PP.PSXC.", 13);
    memcpy(expected + 13, r.save_key, keylen + 1);
    if (strcmp(r.save_partition, expected) || !strcmp(r.save_partition, r.disc_partition))
        return "launch-save-partition";
    /* Empty serial is allowed for unidentified discs; never a title match. */
    if (r.serial[0]) {
        if (strlen(r.serial) != 11 || r.serial[4] != '_' || r.serial[8] != '.') return "launch-serial";
        for (unsigned i = 0; i < 11; ++i) {
            if (i == 4 || i == 8) continue;
            if (i < 4 ? (r.serial[i] < 'A' || r.serial[i] > 'Z') :
                (r.serial[i] < '0' || r.serial[i] > '9')) return "launch-serial";
        }
    }
    if (!nonzero(r.disc_sha256, 32) || !nonzero(r.header_sha1, 20)) return "launch-disc-identity";
    return np_compat_resolve(r.runtime, r.environment, r.serial, r.disc_sha256,
        &r.options, resolved ? resolved : &temporary);
}
const char *np_launch_argument(const NpLaunchRequest *r, char *out, size_t capacity) {
    const char *error = np_launch_check(r, r ? sizeof(*r) : 0, NULL);
    if (out && capacity) out[0] = 0;
    if (error) return error;
    if (!out || capacity <= strlen(r->disc_partition) + strlen(":/disc/disc0"))
        return "launch-argument-size";
    snprintf(out, capacity, "%s:/disc/disc0", r->disc_partition);
    return NULL;
}
