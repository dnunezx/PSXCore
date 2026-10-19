/* Original LUNA return panel. Uses the pinned POPS sprite submission path;
 * no GS reset, display-mode change, input polling or independent DMA owner. */
#include <stdint.h>
#include "menu_art.h"
#define RGB(r,g,b) ((uint32_t)(r)|((uint32_t)(g)<<8)|((uint32_t)(b)<<16)|0x80000000u)
#ifdef NP_MENU_TEST
extern void np_menu_rect(int,int,int,int,uint32_t);
extern uint32_t np_menu_selection(void);
#else
static void np_menu_rect(int x,int y,int w,int h,uint32_t color) {
    ((void (*)(int,int,int,int,uint32_t))(uintptr_t)0x228c80)(x,y,w,h,color);
}
static uint32_t np_menu_selection(void) {return *(volatile uint32_t *)0x85a994;}
#endif
/* LUNA accents on black. Full-coverage GS alpha is 0x80. */
#define INK RGB(0xf0,0xfa,0xff)
#define MUTED RGB(0xc8,0xd4,0xe8)
#define PANEL RGB(0,0,0)
#define EDGE RGB(0x29,0x4e,0x7a)
#define ACCENT RGB(0x33,0xb5,0xe5)
#define HIGHLIGHT RGB(0x12,0x35,0x5d)
static uint32_t blend(uint32_t foreground,uint32_t background,unsigned coverage) {
    uint32_t color=0x80000000u;
    for(unsigned shift=0;shift<24;shift+=8) {
        unsigned f=(foreground>>shift)&255u,b=(background>>shift)&255u;
        color|=((f*coverage+b*(3-coverage)+1)/3)<<shift;
    }
    return color;
}
static void label(int x,int y,uint32_t foreground,uint32_t background,const NpMenuLabel *text) {
    for(unsigned i=0;i<text->count;++i) {
        const NpMenuRun *r=&text->runs[i];
        np_menu_rect(x+r->x,y+r->y,r->w,r->h,blend(foreground,background,r->coverage));
    }
}
static void logo(void) {
    /* Official white wordmark: the same twelve source-pixel bounds used by
     * LUNA's own ambient_orbs.c, derived from luna-logo-white.png (710x109).
     * Scale uniformly to a quarter size; do not replace it with text. */
    static const uint16_t strokes[][4]={
        {0,0,11,106},{0,95,139,106},{191,0,202,106},{318,0,329,59},
        {191,95,329,106},{381,0,520,11},{381,47,393,106},{509,0,520,106},
        {572,0,710,11},{572,47,583,106},{699,0,710,106},{572,47,710,59}
    };
    for(unsigned i=0;i<12;++i) {
        int x=(strokes[i][0]+2)/4,y=(strokes[i][1]+2)/4;
        int right=(strokes[i][2]+2)/4,bottom=(strokes[i][3]+2)/4;
        np_menu_rect(231+x,126+y,right-x,bottom-y,RGB(255,255,255));
    }
}
void np_igr_menu_draw(void) {
    unsigned choice=np_menu_selection();
    np_menu_rect(64,94,512,228,PANEL);
    np_menu_rect(65,95,510,226,EDGE);
    np_menu_rect(67,97,506,222,PANEL);
    logo();
    label(320-(int)np_menu_note.width/2,188,MUTED,PANEL,&np_menu_note);
    for(unsigned i=0;i<2;++i) {
        int x=i?332:104;
        uint32_t background=choice==i?HIGHLIGHT:PANEL;
        const NpMenuLabel *text=i?&np_menu_play:&np_menu_return;
        np_menu_rect(x,238,204,52,choice==i?ACCENT:EDGE);
        np_menu_rect(x+2,240,200,48,background);
        label(x+102-(int)text->width/2,264-(int)text->height/2,
            choice==i?INK:MUTED,background,text);
    }
}
