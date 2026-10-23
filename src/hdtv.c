#include "psxcore_hdtv.h"
#include <string.h>

typedef struct {
    uint32_t address, count, words[4];
} NpHdtvAnchor;
/* Evidence in the pinned image: cached CRT tuple; reset/SetGsCrt wrapper;
 * syscall 2; native SMODE2, DISPLAY geometry and GS publication. These are
 * validation anchors, never additional writes or guessed revision patterns. */
static const NpHdtvAnchor anchors[] = {
    {0x219e14,3,{0x8e42efd0,0x8e240064,0x1444000c}},
    {0x219df8,3,{0x27bdffe0,0xffb10008,0x0080882d}},
    {0x219e78,4,{0x84450000,0x84e60000,0x0c08eaf6,0x85070000}},
    {0x21a050,4,{0x8e240064,0x00c31824,0x2442ffff,0x00821004}},
    {0x21a074,4,{0x3c021200,0x26103280,0x34420020,0xde030000}},
    {0x21a088,2,{0x0c08683c,0xfc430000}},
    {0x21a090,4,{0x3c021200,0xde050010,0x3c031200,0x34420080}},
    {0x21a0a0,2,{0x346300a0,0xfc450000}},
    {0x21a4c0,4,{0xacc80000,0x0000202d,0xace50000,0xac690000}},
    {0x23abd8,2,{0x27bdffb0,0x00042400}},
    {0x23acd0,2,{0x0808b908,0x27bd0050}},
    {0x23ad48,2,{0x0808b908,0x27bd0050}},
    {0x22e420,4,{0x24030002,0x0000000c,0x03e00008,0}},
    {0x23aee0,4,{0x86220000,0x10400005,0x24020002,0x86220004}},
    {0x23aef0,4,{0x14400002,0x24020003,0x24020001,0xfe620008}},
    {0x23af24,2,{0x24040002,0x14a4002b}},
    {0x23afd8,2,{0x24020003,0x14a20043}},
    {0x23b048,4,{0x10c00005,0x30a50fff,0x00121040,0x01431825}}
};

static int matches(const void *image, size_t size, const NpElf *elf,
    uint32_t address, const void *words, size_t length) {
    unsigned found = 0;
    for (unsigned i = 0; i < elf->count; ++i) {
        const NpSegment *s = &elf->segments[i];
        if (address < s->address || (uint64_t)address + length > (uint64_t)s->address + s->filesz)
            continue;
        uint64_t offset = (uint64_t)s->offset + address - s->address;
        if (offset + length > size || memcmp((const unsigned char *)image + offset, words, length))
            return 0;
        ++found;
    }
    return found == 1;
}

const char *np_hdtv_plan(uint32_t runtime, uint32_t fixes,
    const void *image, size_t size, const NpElf *elf, NpPatchSet *set,
    uintptr_t begin, uintptr_t display, uintptr_t crt, uintptr_t height, uintptr_t end) {
    static const uint32_t sites[4] = {0x21b170,0x21b178,0x221bec,0x221d58};
    static const uint32_t expected[4][2] = {
        {0x0c086924,0xacf20000},{0x0c08677e,0x0280202d},
        {0x0c08677e,0x0200202d},{0x0c08677e,0x00a0202d}};
    if (runtime != NP_RUNTIME_KNOWN) return "hdtv-runtime";
    if (!set || set->count > NP_PATCH_LIMIT) return "hdtv-plan-input";
    if (!(fixes & NP_FIX_HDTV)) return NULL;
    if (!image || !elf || !elf->count || elf->count > 8) return "hdtv-image";
    if (begin < 0xf7000 || end <= begin || end > 0xf7800 ||
        display < begin || display >= end || crt < begin || crt >= end ||
        height < begin || height >= end || display == crt || height == display || height == crt ||
        ((begin | display | crt | height | end) & 3)) return "hdtv-resident";
    for (unsigned i = 0; i < sizeof(anchors)/sizeof(anchors[0]); ++i)
        if (!matches(image,size,elf,anchors[i].address,anchors[i].words,anchors[i].count*4))
            return "hdtv-control-path";
    NpPatchSet next = *set;
    for (unsigned i = 0; i < 4; ++i) {
        if (!matches(image,size,elf,sites[i],expected[i],8)) return "hdtv-expected";
        uint32_t replacement[2] = {
            0x0c000000u | ((uint32_t)(i ? display : crt) >> 2), expected[i][1]};
        const char *error = np_patch_add(&next,sites[i],expected[i],replacement,8);
        if (error) return error;
    }
    /* The custom-window path would replace 2*height-1 with 2*(height-1).
     * Only our private descriptor needs the low height bit restored. */
    const uint32_t geometry_expected[2]={0x2442ffff,0x00821004};
    const uint32_t geometry_replacement[2]={0x08000000u|((uint32_t)height>>2),0};
    const char *error=np_patch_add(&next,0x21a058,geometry_expected,geometry_replacement,8);
    if(error)return error;
    *set = next;
    return NULL;
}
