/* Original bounded EE/GS-only startup display. No FILEIO/RPC or resident hooks. */
#include "startup_diag.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifndef NP_HOST_TEST
#include <debug.h>
#endif
#ifndef NP_STARTUP_BUILD
#define NP_STARTUP_BUILD "startup-20261012-r7"
#endif
enum { WIDTH=68, ROWS=22 };
static char previous[ROWS][WIDTH],frame[ROWS][WIDTH];
static char operation[137],completed[137],selection[137],path_text[205];
static char error_text[137],context[205],environment_text[69];
static unsigned stage_number;
static int ready,stopped;
static void copy(char *out,size_t size,const char *in) {
    snprintf(out,size,"%s",in?in:"");
}
static void put(unsigned row,unsigned lines,const char *text) {
    unsigned col=0;
    for(;*text && lines && row<ROWS;++text) {
        unsigned char ch=(unsigned char)*text;
        if(ch=='\n' || col==WIDTH) {
            ++row;--lines;col=0;
            if(!lines || row==ROWS)break;
            if(ch=='\n')continue;
        }
        frame[row][col++]=(ch>=32 && ch<127)?(char)ch:'?';
    }
}
static void draw(void) {
    if(!ready)return;
    char title[69];memset(frame,' ',sizeof(frame));
    put(0,1,stopped?"PSXCore startup stopped":"PSXCore startup");
    put(1,1,NP_STARTUP_BUILD);
    snprintf(title,sizeof(title),"Stage %02u: %s",stage_number,stopped?"ERROR":"BEGIN / waiting for return");
    put(3,1,title);put(4,2,operation);
    put(7,2,selection);put(9,1,environment_text);put(10,3,path_text);
    put(14,1,"Last completed:");put(15,2,completed);
    if(error_text[0]) {put(17,1,"Error:");put(18,2,error_text);}
    put(error_text[0]?20:19,error_text[0]?2:3,context);
    for(unsigned y=0;y<ROWS;++y)for(unsigned x=0;x<WIDTH;++x)
        if(previous[y][x]!=frame[y][x]) {
#ifndef NP_HOST_TEST
            scr_putchar((x+6)*8,(y+2)*8,stopped?0x8080ff:0xffffff,(unsigned char)frame[y][x]);
#endif
            previous[y][x]=frame[y][x];
        }
}
void np_startup_init(void) {
#ifndef NP_HOST_TEST
    init_scr();scr_setCursor(0);
#endif
    memset(previous,0,sizeof(previous));ready=1;stopped=0;stage_number=0;
    operation[0]=completed[0]=selection[0]=path_text[0]=error_text[0]=context[0]=environment_text[0]=0;
    draw();
}
void np_startup_begin(unsigned stage,const char *op) {
    stage_number=stage;np_startup_operation(op);
}
void np_startup_operation(const char *op) {
    copy(operation,sizeof(operation),op);if(!error_text[0])context[0]=0;draw();
}
void np_startup_done(void) {copy(completed,sizeof(completed),operation);draw();}
void np_startup_selection(const char *backend,unsigned device,const char *save_backend,
        unsigned save_device,unsigned environment,const char *path) {
    snprintf(selection,sizeof(selection),"Disc: %s device %u; saves: %s device %u",backend,device,save_backend,save_device);
    snprintf(environment_text,sizeof(environment_text),"Environment: %s",environment==1?"physical PS2":"unknown");
    copy(path_text,sizeof(path_text),path);draw();
}
void np_startup_context(const char *format,...) {
    if(error_text[0])return;
    va_list args;va_start(args,format);vsnprintf(context,sizeof(context),format,args);va_end(args);draw();
}
void np_startup_error(const char *code) {
    if(!error_text[0]) {copy(error_text,sizeof(error_text),code);draw();}
}
void np_startup_stop(const char *fallback) {np_startup_error(fallback);stopped=1;memset(previous,0,sizeof(previous));draw();}
