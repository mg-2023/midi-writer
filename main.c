#include "parser.h"
#include "midiwriter.h"

#define INPUT_NAME "input.txt"
#define OUTPUT_NAME "output.mid"

int main(void) {
	uint8_t buf[4];
	uint8_t trackSizeBytes[4];
	
	int trackSize = 0;
	int ppqn;
	int program, tempo, volume;
	int noteLen=0, restLen=0;
	
	FILE *in = fopen(INPUT_NAME, "rt");
	FILE *out = fopen(OUTPUT_NAME, "wb");
	
	if(in == NULL) {
		perror("Error opening file " INPUT_NAME);
		return 1;
	}
	
	fscanf(in, "%d ", &ppqn);
	if(ppqn > 0xffff) {
		fprintf(stderr, "Too much pulses per quarter note\n");
		exit(1);
	}
	
	WriteMidiHeader(ppqn, out);
	WriteTrackHeader(out);
	fwrite(&trackSize, sizeof(int), 1, out);
	fscanf(in, "%d %d ", &tempo, &volume);
	
	trackSize += WriteTrackName(out);
	trackSize += WriteSysEx_GS_Reset(out);
	
	trackSize += WriteControlChange(0, 0, 7, volume, out);
	trackSize += WriteControlChange(0, 0, 0, 0, out);
	trackSize += WriteControlChange(0, 0, 32, 0, out);
	trackSize += WriteProgramChange(0, 0, 0, out);
	
	trackSize += WriteTempo(0, tempo, out);
	
	while(!feof(in)) {
		int res = ReadSingleNote(in, buf);
		if(res == NOTE_TOO_LONG) {
			fprintf(stderr, "On file pos %ld: Note info too long\n", ftell(in));
			exit(1);
		}
		
		int noteNum = ParseNoteInfo(buf);
		if(noteNum == INVALID_CHAR || noteNum == NOTE_OVERFLOW) {
			fprintf(stderr, "On file pos %ld: ", ftell(in));
			if(noteNum == INVALID_CHAR)
				fprintf(stderr, "Invalid character detected while parsing note info\n");
			
			else if(noteNum == NOTE_OVERFLOW) {
				fprintf(stderr, "Too high note key; please use note below C10 (C0~B9)\n");	
			}
			exit(1);
		}
		
		fscanf(in, "%d ", &noteLen);
		
		if(noteNum == 127) {
			restLen += noteLen;
		}
		
		else {
			trackSize += WriteNote(0, 0, noteNum, 100, out);
			trackSize += WriteNote(0, noteLen, noteNum, 0, out);
			restLen = 0;
		}
	}
	
	trackSize += WriteTrackEnd(out);
	
	fseek(out, 18, SEEK_SET);
	GetBytes(trackSizeBytes, trackSize, 4);
	for(int i=0; i<4; i++) {
		fwrite(trackSizeBytes+i, sizeof(uint8_t), 1, out);
	}
	
	fclose(in);
	fclose(out);
	
	return 0;
}

