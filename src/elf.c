/* Original bounded ELF32/MIPS validator; all fields read without alignment assumptions. */
#include "psxcore_elf.h"
#include <string.h>
static uint32_t u32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static unsigned u16(const unsigned char *p) { return p[0] | (unsigned)p[1]<<8; }
const char *np_elf_check(const void *bytes, size_t size, NpElf *out) {
    NpElf result = {0};
    const unsigned char *p = bytes;
    if (!p || !out || size < 52 || size > NP_ELF_MAX_BYTES) return "file-size";
    if (memcmp(p, "\177ELF", 4) || p[4]!=1 || p[5]!=1 || p[6]!=1 ||
        p[7]!=0 || u16(p+16)!=2 || u16(p+18)!=8 || u32(p+20)!=1 ||
        u16(p+40)!=52 || u16(p+42)!=32) return "elf-header";
    unsigned n = u16(p+44); uint32_t ph = u32(p+28);
    if (!n || n > 32 || ph < 52 || ph > size || n > (size-ph)/32) return "program-headers";
    result.entry = u32(p+24);
    int executableEntry = 0;
    if (result.entry & 3) return "entry-alignment";
    for (unsigned i=0; i<n; ++i) {
        const unsigned char *h = p+ph+i*32;
        uint32_t type=u32(h), off=u32(h+4), addr=u32(h+8), phys=u32(h+12);
        uint32_t files=u32(h+16), mem=u32(h+20), flags=u32(h+24), align=u32(h+28);
        /* SDK reginfo is metadata. Dynamic/interpreter/TLS semantics are unsupported. */
        if (type==0) continue;
        if (type==0x70000000u || type==0x70000003u) {
            if (off>size || files>size-off) return "metadata-size";
            continue;
        }
        if (type!=1) return "unsupported-segment";
        if (!mem || files>mem || off>size || files>size-off) return "segment-size";
        if (addr<NP_ELF_LOW || addr>=NP_ELF_HIGH || mem>NP_ELF_HIGH-addr || phys!=addr)
            return "segment-address";
        if (flags & ~7u) return "segment-flags";
        if (align>1 && ((align & (align-1)) || (addr & (align-1)) != (off & (align-1))))
            return "segment-alignment";
        if (result.count==NP_ELF_MAX_SEGMENTS) return "segment-count";
        for (unsigned j=0; j<result.count; ++j) {
            NpSegment *s=&result.segments[j];
            if (addr<s->address+s->memsz && s->address<addr+mem) return "segment-overlap";
        }
        result.segments[result.count++] = (NpSegment){off,addr,files,mem};
        if ((flags & 1) && result.entry>=addr && result.entry-addr<files) executableEntry=1;
    }
    if (!result.count || !executableEntry) return "entry-unmapped";
    *out = result; /* No partial plan escapes on rejection. */
    return NULL;
}
