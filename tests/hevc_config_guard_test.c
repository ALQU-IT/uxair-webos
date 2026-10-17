#include "hevc_config_guard.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t data[0x75+3*7]={0};
    size_t pos=0x75;
    for(int i=0;i<3;++i) {
        data[pos++]=(uint8_t)(0xa0+i);data[pos++]=0;data[pos++]=1;
        data[pos++]=0;data[pos++]=2;
        data[pos++]=(uint8_t)((32+i)<<1);data[pos++]=1;
    }
    ux_hevc_config_sets parsed;
    assert(ux_hevc_config_parse(data,sizeof(data),&parsed));
    assert(parsed.annexb_size==18 && parsed.offset[0]==0x75+5);
    for(size_t n=0;n<sizeof(data);++n) assert(!ux_hevc_config_parse(data,n,&parsed));
    data[0x75+3]=0xff;data[0x75+4]=0xff;
    assert(!ux_hevc_config_parse(data,sizeof(data),&parsed));
    data[0x75+3]=0;data[0x75+4]=2;
    data[0x75+5]=0x80;
    assert(!ux_hevc_config_parse(data,sizeof(data),&parsed));
    data[0x75+5]=32<<1;data[0x75+6]=0;
    assert(!ux_hevc_config_parse(data,sizeof(data),&parsed));
    assert(!ux_hevc_config_parse(NULL,sizeof(data),&parsed));
    assert(!ux_hevc_config_parse(data,sizeof(data),NULL));
    puts("PASS: bounded pinned hvc1 parameter-set parser");
}
