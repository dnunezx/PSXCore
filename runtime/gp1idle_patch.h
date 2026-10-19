/* Pinned native contracts used by the parked-worker control path. */
#ifndef NP_GP1IDLE_PATCH_H
#define NP_GP1IDLE_PATCH_H
_Static_assert(sizeof(ee_sema_t)==24 && offsetof(ee_sema_t,count)==0 &&
               offsetof(ee_sema_t,max_count)==4 && offsetof(ee_sema_t,wait_threads)==12,
               "interrupt-safe semaphore status layout");
extern char np_gp1_idle_begin[],np_gp1_idle_end[],np_gp1_idle_lease[];
extern void np_gp1_idle_control(void),np_gp1_idle_park(void);
static int install_gp1_idle_control(const NpElf *elf) {
    uintptr_t begin=(uintptr_t)np_gp1_idle_begin,end=(uintptr_t)np_gp1_idle_end;
    uintptr_t control=(uintptr_t)np_gp1_idle_control,park_entry=(uintptr_t)np_gp1_idle_park;
    uintptr_t lease=(uintptr_t)np_gp1_idle_lease;
    if(begin!=0xfec00 || control!=begin || end<=begin || end>0xff000 ||
       park_entry<=begin || park_entry>=lease || lease+4!=end || ((begin|park_entry|lease)&3))return -1;
    if(*(const uint32_t *)lease)return -1;
    static const uint32_t caller[]={
        0x0c08695au,0x00a0202du,0xdfbf0010u,0x03e00008u,
    };
    static const uint32_t prologue[]={
        0x27bdffe0u,0xffb10008u,0x0080882du,0x00111e02u,
    };
    static const uint32_t publish[]={
        0x8c822284u,0x00021880u,0x24420001u,0x2c450020u,
        0x00641821u,0x0005100au,0xac712040u,0xac822284u,
    };
    static const uint32_t consume[]={
        0x8e032280u,0x00000000u,0x00031080u,0x24630001u,
        0x2c640020u,0x00501021u,0x0004180au,0x8c442040u,
        0xae032280u,0x40026000u,0x00521024u,0x10400009u,
        0x00000000u,0x3c030001u,0x42000039u,0x0000040fu,
        0x40026000u,0x00431024u,0x00000000u,0x1440fffau,
        0x00000000u,0x8e02227cu,0x2442ffffu,0xae02227cu,
        0x0c0885c2u,0x42000038u,0x8e02227cu,0x5440ffe6u,
        0x8e032280u,0x8e2222acu,0x10400004u,0x266310f0u,
        0x0c088728u,0x00000000u,
    };
    static const uint32_t direction[]={
        0x8e24223cu,0x3c029fffu,0x32450003u,0x3442ffffu,
        0x00822024u,0x00051f40u,0x00832025u,0x10a00008u,
        0xae24223cu,0x8e22223cu,0x3c030200u,0x00431025u,
        0xae22223cu,0x100000f5u,0xdfb00000u,0x00000000u,
        0x8e23223cu,0x3c02fdffu,0x3442ffffu,0x00621824u,
        0xae23223cu,0x100000edu,0xdfb00000u,0x00000000u,
    };
    static const uint32_t park[]={
        0x0c08ba10u,0x8e042b18u,0x8e0322bcu,0x04610005u,
        0x00000000u,0x8e02223cu,0x10000004u,0x3c031400u,
        0x00000000u,0x8e02223cu,0x3c031000u,0x00431025u,
        0xae02223cu,0x0c08ba18u,0x8e242b14u,0x0c08ba1cu,
        0x8e242b18u,0x8e23223cu,0x3c02ebffu,0x3442ffffu,
        0x00621824u,0x1000ff71u,0xae23223cu,
    };
    static const struct { uint32_t address;const uint32_t *words;uint32_t bytes; } pins[]={
        {0x20b518u,caller,sizeof(caller)},
        {0x21a568u,prologue,sizeof(prologue)},
        {0x21a604u,publish,sizeof(publish)},
        {0x21c458u,consume,sizeof(consume)},
        {0x221878u,direction,sizeof(direction)},
        {0x21c61cu,park,sizeof(park)}
    };
    for(unsigned p=0;p<sizeof(pins)/sizeof(pins[0]);++p) {
        int found=0;
        for(unsigned i=0;i<elf->count;++i) {
            const NpSegment *s=&elf->segments[i];
            if(s->address<=pins[p].address &&
               pins[p].address+pins[p].bytes<=s->address+s->filesz) {
                const void *actual=(const void *)(NP_BOOT_STAGE+s->offset+pins[p].address-s->address);
                if(memcmp(actual,pins[p].words,pins[p].bytes))return -1;
                found=1;break;
            }
        }
        if(!found)return -1;
    }
    const uint32_t expected[2]={0x0080882d,0x00111e02};
    const uint32_t replacement[2]={0x08000000u|((uint32_t)control>>2),0};
    return np_patch_add(&patch_set,0x21a570,expected,replacement,8) ||
           queue_word(0x21c650,0x0c08ba18,0x0c000000u|((uint32_t)park_entry>>2))?-1:0;
}
#endif
