#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

static inline void AssertDeltaTime(int delta) {
    if(delta > 0xfffffff) {
        fprintf(stderr, "Given deltatime length too long: %d\n", delta);
        exit(1);
    }
}

void GetBytes(uint8_t *bytes, int32_t num, int32_t byteLen) {
    for(int i=byteLen-1; i>=0; i--) {
        bytes[i] = num&0xff;
        num >>= 8;
    }
}

void GetBytesLong(uint8_t *bytes, int64_t num, int32_t byteLen) {
    for(int i=byteLen-1; i>=0; i--) {
        bytes[i] = num&0xff;
        num >>= 8;
    }
}

/* WARNING: Windows Media Player would crash
 *          when navigating through very long midi file!!
 * maximum delta time:      28-bit full integer(268435455)
 * maximum delta time byte: FF FF FF 7F
 */
int32_t ConvertToDeltaTime(uint8_t *deltaBytes, int32_t deltaTime) {
    int32_t deltaLen;
    int32_t bytes[4];
    bytes[0] = deltaTime & 0x7f;              // 00000000 00000000 00000000 0ddddddd
    bytes[1] = (deltaTime & 0x3f80) >> 7;     // 00000000 00000000 00cccccc c0000000
    bytes[2] = (deltaTime & 0x1fc000) >> 14;  // 00000000 000bbbbb bb000000 00000000
    bytes[3] = (deltaTime & 0xfe00000) >> 21; // 0000aaaa aaa00000 00000000 00000000
    
    // 4-byte delta time bit: 1aaaaaaa 1bbbbbbb 1ccccccc 0ddddddd
    if(bytes[3] > 0) {
        deltaLen = 4;
    }
    
    // 3-byte delta time bit: 1bbbbbbb 1ccccccc 0ddddddd
    else if(bytes[2] > 0) {
        deltaLen = 3;
    }
    
    // 2-byte delta time bit: 1ccccccc 0ddddddd
    else if(bytes[1] > 0) {
        deltaLen = 2;
    }
    
    // 1-byte delta time bit: 0ddddddd
    else {
        deltaLen = 1;
    }
    
    for(int i=0; i<deltaLen; i++) {
        if(i == deltaLen-1) {
            deltaBytes[i] = bytes[deltaLen-i - 1];
        }
        
        else {
            deltaBytes[i] = 0x80 + bytes[deltaLen-i - 1];
        }
    }
    
    return deltaLen;
}

// header indicates that this midi file would be
// midi file type 0(single track), [PPQN] pulses per quarter note
void WriteMidiHeader(int32_t PPQN, FILE *fout) {
    uint8_t headerInfo[14] = {'M', 'T', 'h', 'd',
                            0x00, 0x00, 0x00, 0x06,
                            0x00, 0x00, 0x00, 0x01};
    
    GetBytes(headerInfo+12, PPQN, 2);
    for(int i=0; i<14; i++) {
        fwrite(headerInfo+i, sizeof(uint8_t), 1, fout);
    }
}

void WriteTrackHeader(FILE *fout) {
    uint8_t headerInfo[] = {'M', 'T', 'r', 'k'};
    
    for(int i=0; i<4; i++) {
        fwrite(headerInfo+i, sizeof(uint8_t), 1, fout);
    }
}

/* this SysEx message allows to use GS sound set beyond GM's 128 instruments
 * message: 00 F0 0A 41 7F 42 12 40 00 7F 00 41 F7
 *
 * NOTE: after this SysEx message send bank change first (both MSB and LSB)
 * before sending program change
 */
int32_t WriteSysEx_GS_Reset(FILE *fout) {
    uint8_t SysEx[] = {0x00, 0xf0, 0x0a,
                       0x41, 0x7f, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};
    
    for(int i=0; i<13; i++) {
        fwrite(SysEx+i, sizeof(uint8_t), 1, fout);
    }
    
    return 13;
}

int32_t WriteTrackName(FILE *fout) {
    uint8_t Meta[] = {0x00, 0xff, 0x03, 0x05, 'T', 'r', 'a', 'c', 'k'};
    
    for(int i=0; i<9; i++) {
        fwrite(Meta+i, sizeof(uint8_t), 1, fout);
    }
    
    return 9;
}

/* midi tempo message: FF 51 03 [3-byte integer]
 * where integer is microseconds per beat
 * e.g. FF 51 03 07 A1 20:
 * 0x7a120 = 500000, 500000us = 0.5s
 * 0.5 seconds per beat = 120 BPM
 */
int32_t WriteTempo(int32_t delay, int32_t BPM, FILE *fout) {
    AssertDeltaTime(delay);
    uint8_t deltaBytes[4];
    int32_t deltaBytesLen = ConvertToDeltaTime(deltaBytes, delay);
    
    uint8_t tempoData[6] = {0xff, 0x51, 0x03};
    int32_t us = (int32_t)(1e6 * (60.0/BPM));
    
    GetBytes(tempoData+3, us, 3);
    for(int i=0; i<deltaBytesLen; i++) {
        fwrite(deltaBytes+i, sizeof(uint8_t), 1, fout);
    }
    for(int i=0; i<6; i++) {
        fwrite(tempoData+i, sizeof(uint8_t), 1, fout);
    }
    
    return 6 + deltaBytesLen;
}

/* midi control change: Bc v1 v2
 * where c is channel(0 to F, Ch1 to Ch16 respectively),
 * v1 is control number, v2 is control value (00 to 7F)
 *
 * commonly used v1 value:
 * 0x00, 0x20(bank MSB/LSB), 0x07(volume), 0x0A(pan), 0x40(sustain pedal)
 */
int32_t WriteControlChange(int32_t delay, int32_t chn, int32_t cc, int32_t value, FILE *fout) {
    AssertDeltaTime(delay);
    uint8_t deltaBytes[4];
    int32_t deltaBytesLen = ConvertToDeltaTime(deltaBytes, delay);
    
    uint8_t ccData[] = {0xb0+chn, cc, value};
    for(int i=0; i<deltaBytesLen; i++) {
        fwrite(deltaBytes+i, sizeof(uint8_t), 1, fout);
    }
    for(int i=0; i<3; i++) {
        fwrite(ccData+i, sizeof(uint8_t), 1, fout);
    }
    
    return 3 + deltaBytesLen;
}

/* program change: Cc pp
 * where c is channel(0 to F, Ch1 to Ch16 respectively),
 * pp is program number(00 to 7F)
 * e.g. C0 00: set channel 1's program to #0 (Piano 1)
 */
int32_t WriteProgramChange(int32_t delay, int32_t chn, int32_t program, FILE *fout) {
    AssertDeltaTime(delay);
    uint8_t deltaBytes[4];
    int32_t deltaBytesLen = ConvertToDeltaTime(deltaBytes, delay);
    
    uint8_t pcData[] = {0xc0+(uint8_t)chn, (uint8_t)program};
    for(int i=0; i<deltaBytesLen; i++) {
        fwrite(deltaBytes+i, sizeof(uint8_t), 1, fout);
    }
    for(int i=0; i<2; i++) {
        fwrite(pcData+i, sizeof(uint8_t), 1, fout);
    }
    
    return 2 + deltaBytesLen;
}

/* note message: 9c nn vv
 * where c is channel(0 to F, Ch1 to Ch16 respectively),
 * nn is note number(00 to 7F), vv is velocity (00 to 7F)
 * if velocity is 0 this corresponds to note OFF message
 */
int32_t WriteNote(int32_t delay, int32_t chn, int32_t noteNum, int32_t vel, FILE *fout) {
    AssertDeltaTime(delay);
    uint8_t deltaBytes[4];
    int32_t deltaBytesLen = ConvertToDeltaTime(deltaBytes, delay);
    
    uint8_t noteOnData[] = {0x90+chn, noteNum, vel};
    for(int i=0; i<deltaBytesLen; i++) {
        fwrite(deltaBytes+i, sizeof(uint8_t), 1, fout);
    }
    for(int i=0; i<3; i++) {
        fwrite(noteOnData+i, sizeof(uint8_t), 1, fout);
    }
    
    return 3 + deltaBytesLen;
}

/* pitch bend message: Ec v1 v2
 * where c is channel(0 to F, Ch1 to Ch16 respectively),
 * either v1 or v2 is 00 to 7F
 * 
 * available bend value is 0 to 16383 (14bit full integer)
 * bend value = v2*128 + v1
 */
int32_t WritePitchBend(int32_t delay, int32_t chn, int32_t value, FILE *fout) {
    AssertDeltaTime(delay);
    uint8_t deltaBytes[4];
    int32_t deltaBytesLen = ConvertToDeltaTime(deltaBytes, delay);

    uint8_t pitchBendData[] = {0xe0+chn, 0, 0};
    pitchBendData[1] = value&0x7f;
    pitchBendData[2] = (value&0x3f80) >> 7;
    for(int i=0; i<deltaBytesLen; i++) {
        fwrite(deltaBytes+i, sizeof(uint8_t), 1, fout);
    }
    for(int i=0; i<3; i++) {
        fwrite(pitchBendData+i, sizeof(uint8_t), 1, fout);
    }
    
    return 3 + deltaBytesLen;
}

int32_t WriteTrackEnd(FILE *fout) {
    uint8_t EOT[4] = {0x00, 0xff, 0x2f, 0x00};
    for(int i=0; i<4; i++) {
        fwrite(EOT+i, sizeof(uint8_t), 1, fout);
    }
    
    return 4;
}

