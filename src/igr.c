#include "psxcore_igr.h"
#include <string.h>

static int path_character(unsigned char c) {
    return (c>='A' && c<='Z') || (c>='a' && c<='z') ||
        (c>='0' && c<='9') || c=='_' || c=='-' || c=='.' || c==' ';
}
int np_igr_path_valid(const char *path,size_t capacity) {
    if(!path || capacity<7 || capacity>NP_IGR_PATH_BYTES)return 0;
    const char *end=memchr(path,0,capacity);
    if(!end || end-path<6 || path[0]!='m' || path[1]!='c' ||
       (path[2]!='0' && path[2]!='1') || path[3]!=':' || path[4]!='/')return 0;
    const char *component=path+5;
    for(const char *p=component;;++p) {
        if(p==end || *p=='/') {
            size_t n=(size_t)(p-component);
            if(!n || (n==1 && component[0]=='.') ||
               (n==2 && component[0]=='.' && component[1]=='.') ||
               component[0]==' ' || component[n-1]==' ')return 0;
            if(p==end)return 1;
            component=p+1;
        } else if(!path_character((unsigned char)*p))return 0;
    }
}
void np_igr_config_init(NpIgrConfig *config) {
    if(!config)return;
    memset(config,0,sizeof(*config));
    config->magic=NP_IGR_MAGIC;
    config->version=NP_IGR_VERSION;
    config->size=sizeof(*config);
}
const char *np_igr_config_check(const void *bytes,size_t size) {
    NpIgrConfig config;
    if(!bytes || size!=sizeof(config))return "igr-config-size";
    memcpy(&config,bytes,sizeof(config));
    if(config.magic!=NP_IGR_MAGIC || config.version!=NP_IGR_VERSION ||
       config.size!=sizeof(config))return "igr-config-version";
    if(config.enabled>1 || config.reserved)return "igr-config-flags";
    const char *end=memchr(config.return_path,0,sizeof(config.return_path));
    if(!end)return "igr-config-path";
    if(config.enabled) {
        if(!np_igr_path_valid(config.return_path,sizeof(config.return_path)))return "igr-config-path";
    } else if(end!=config.return_path)return "igr-config-disabled-path";
    for(const char *p=end;p<config.return_path+sizeof(config.return_path);++p)
        if(*p)return "igr-config-padding";
    return NULL;
}
const char *np_igr_input_init(NpIgrInput *state,const NpIgrConfig *config) {
    if(!state)return "igr-input-output";
    /* Copy before clearing state so even overlapping caller storage is safe. */
    NpIgrConfig copy;
    if(config)memcpy(&copy,config,sizeof(copy));
    memset(state,0,sizeof(*state));
    const char *error=np_igr_config_check(config?&copy:NULL,sizeof(copy));
    if(error)return error;
    state->enabled=copy.enabled;
    return NULL;
}
static void disarm(NpIgrInput *state) {
    state->armed=0;
    state->holding=0;
}
uint32_t np_igr_input_update(NpIgrInput *state,uint16_t buttons,
    uint32_t sequence,uint32_t now_ms,int valid) {
    if(!state || state->enabled!=1)return 0;
    if(valid!=1) {disarm(state);return 0;}
    uint32_t result=(buttons&NP_IGR_COMBO)==NP_IGR_COMBO?NP_IGR_INPUT_RESERVED:0;
    if(state->have_report && (uint32_t)(now_ms-state->report_ms)>NP_IGR_REPORT_GAP_MS)
        disarm(state);
    if(state->have_report && sequence==state->sequence)
        return result;
    state->have_report=1;
    state->sequence=sequence;
    state->report_ms=now_ms;
    if(state->requested)return result;
    if(!(buttons&NP_IGR_COMBO)) {
        state->armed=1;
        state->holding=0;
        return result;
    }
    if(!result) {state->holding=0;return result;}
    if(!state->armed)return result;
    if(!state->holding) {
        state->holding=1;
        state->hold_ms=now_ms;
    } else if((uint32_t)(now_ms-state->hold_ms)>=NP_IGR_HOLD_MS) {
        state->requested=1;
        state->holding=0;
        result|=NP_IGR_INPUT_REQUEST;
    }
    return result;
}
