/* Original PSXCore request validation, 2026. */
#include "psxcore_request.h"
#include <stdio.h>
#include <string.h>

static int copy_field(char *out, size_t size, const char *in) {
    if (!in || strlen(in) >= size) return 0;
    memcpy(out, in, strlen(in) + 1);
    return 1;
}

static int source_valid(const NpRequest *r) {
    if (strcmp(r->device, "ata0") || strncmp(r->partition, "hdd0:", 5)) return 0;
    size_t n = strlen(r->partition + 5);
    if (!n || n > 32) return 0;
    for (const unsigned char *p = (const unsigned char *)r->partition + 5; *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-' || *p == '.')) return 0;
    n = strlen(r->path);
    if (n < 5 || r->path[0] != '/' || r->path[n-4] != '.' ||
        (r->path[n-3] != 'v' && r->path[n-3] != 'V') ||
        (r->path[n-2] != 'c' && r->path[n-2] != 'C') ||
        (r->path[n-1] != 'd' && r->path[n-1] != 'D')) return 0;
    const char *start = r->path + 1;
    for (const unsigned char *p = (const unsigned char *)start; ; ++p) {
        if (*p == '/' || !*p) {
            size_t len = (const char *)p - start;
            if (!len || (len == 1 && start[0] == '.') ||
                (len == 2 && start[0] == '.' && start[1] == '.')) return 0;
            if (!*p) break;
            start = (const char *)p + 1;
        } else if (*p < 32 || *p == 127 || *p == ':' || *p == '\\') return 0;
    }
    return 1;
}

const char *np_request_check(const void *bytes, size_t length) {
    NpRequest r;
    if (!bytes || length != sizeof(r)) return "request-size";
    memcpy(&r, bytes, sizeof(r)); /* Also accepts unaligned input safely. */
    if (r.magic != NP_REQUEST_MAGIC || r.version != NP_REQUEST_VERSION ||
        r.size != sizeof(r)) return "request-version";
    if (r.platform != 1 || r.backend != 1 || r.flags != NP_REQUEST_DRY_RUN || r.reserved)
        return "request-kind";
    if (!memchr(r.device, 0, sizeof(r.device)) || !memchr(r.partition, 0, sizeof(r.partition)) ||
        !memchr(r.path, 0, sizeof(r.path)) || !memchr(r.identity, 0, sizeof(r.identity)))
        return "request-string";
    if (!source_valid(&r)) return "request-source";
    char key[sizeof(r.identity)];
    int n = snprintf(key, sizeof(key), "ps1:apa:%s:%s", r.partition, r.path);
    if (n < 0 || (size_t)n >= sizeof(key) || strcmp(key, r.identity)) return "request-identity";
    return NULL;
}

const char *np_request_build(NpRequest *out, const char *device,
                             const char *partition, const char *path) {
    if (!out) return "request-output";
    memset(out, 0, sizeof(*out));
    out->magic = NP_REQUEST_MAGIC; out->version = NP_REQUEST_VERSION;
    out->size = sizeof(*out); out->platform = 1; out->backend = 1;
    out->flags = NP_REQUEST_DRY_RUN;
    if (!copy_field(out->device, sizeof(out->device), device) ||
        !copy_field(out->partition, sizeof(out->partition), partition) ||
        !copy_field(out->path, sizeof(out->path), path)) return "request-truncation";
    int n = snprintf(out->identity, sizeof(out->identity), "ps1:apa:%s:%s", partition, path);
    if (n < 0 || (size_t)n >= sizeof(out->identity)) return "request-truncation";
    return np_request_check(out, sizeof(*out));
}
