#include "midiwriter.h"
#include <time.h>

#define PPQN 480

int main(void) {
    FILE *out = fopen("autogen.mid", "wb");
    int32_t trackSize = 0;
    uint8_t trackByte[4] = {0};
    
    WriteMidiHeader(PPQN, out);
    WriteTrackHeader(out);
    for(int i=0; i<4; i++) {
        fwrite(trackByte+i, sizeof(uint8_t), 1, out);
    }
    
    trackSize += WriteTrackName(out);
    trackSize += WriteSysEx_GS_Reset(out);
    trackSize += WriteControlChange(0, 9, 7, 100, out);
    trackSize += WriteControlChange(0, 9, 0, 0, out);
    trackSize += WriteControlChange(0, 9, 32, 0, out);
    trackSize += WriteProgramChange(0, 9, 80, out);
    trackSize += WriteTempo(0, 120, out);
    
    int offset = 0;
    bool add = true;
    srand(time(NULL));
    for(int i=0; i<256; i++) {
        offset = rand()%25;
        trackSize += WriteNote(9, 0, 35+offset, 100, out);
        
        trackSize += WriteNote(9, PPQN/4, 35+offset, 0, out);
        
        /*
        if(offset == 16) add = false;
        else if(offset == 0) add = true;
        offset += (add) ? 1 : -1;
        */
    }
    trackSize += WriteTrackEnd(out);
    
    GetBytes(trackByte, trackSize, 4);
    fseek(out, 18, SEEK_SET);
    for(int i=0; i<4; i++) {
        fwrite(trackByte+i, sizeof(uint8_t), 1, out);
    }
    fclose(out);
    
    return 0;
}

