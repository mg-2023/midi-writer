#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _STRING_H
#include <string.h>
#endif

#ifndef _CTYPE_H
#include <ctype.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

#define SUCCESS        0
#define INVALID_CHAR  -1
#define NOTE_OVERFLOW -2
#define NOTE_TOO_LONG -3

int ReadSingleNote(FILE *fin, char *buf) {
    int pos = 0;
    memset(buf, 0, sizeof(char)*4);
    while(true) {
        if(pos > 4) {
            return NOTE_TOO_LONG;
        }
        
        char ch = fgetc(fin);
        if(ch == -1 || isspace(ch)) {
            return SUCCESS;
        }
        
        else {
            buf[pos] = ch;
        }
        pos++;
    }
}

int ParseNoteInfo(char *buf) {
    int noteNum;
    int parsePos = 0;
    
    switch(buf[parsePos]) {
        case 'C':
        case 'c':
            noteNum = 0; break;
        
        case 'D':
        case 'd':
            noteNum = 2; break;
            
        case 'E':
        case 'e':
            noteNum = 4; break;
            
        case 'F':
        case 'f':
            noteNum = 5; break;
        
        case 'G':
        case 'g':
            noteNum = 7; break;
        
        case 'A':
        case 'a':
            noteNum = 9; break;
        
        case 'B':
        case 'b':
            noteNum = 11; break;
        
        case 'R':
        case 'r':
            return 127;
        
        default:
            return INVALID_CHAR;
    }
    
    parsePos++;
    if(buf[parsePos] == '#') {
        noteNum++;
        parsePos++;
    }
    
    else if(!isdigit(buf[parsePos])) {
        return INVALID_CHAR;
    }
    
    if(isdigit(buf[parsePos])) {
        char *temp = buf+parsePos;
        if(*(temp+1) != 0) {
            return NOTE_OVERFLOW;
        }
        
        else {
            noteNum += (*temp-'0') * 12;
        }
    }
    
    return noteNum;
}

