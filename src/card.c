#include "psxcore_file.h"
#include <string.h>

int np_file_card_format_frame(unsigned frame,unsigned char bytes[128]) {
    if(!bytes || frame>=NP_FILE_CARD_BYTES/128)return 0;
    memset(bytes,0,128);
    if(frame==0 || frame==63){bytes[0]='M';bytes[1]='C';}
    else if(frame>=1 && frame<=15){bytes[0]=0xa0;bytes[8]=bytes[9]=0xff;}
    else if(frame>=16 && frame<=35){memset(bytes,0xff,4);bytes[8]=bytes[9]=0xff;}
    for(unsigned index=0;index<127;++index)bytes[127]^=bytes[index];
    return 1;
}
