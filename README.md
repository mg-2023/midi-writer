# midi-writer
 Midi file writer

## How to use main.c
 main.c operates with "input.txt" text file
 
 file structure is as follows:
 ```
 [ticks per quarter note] [tempo] [volume]
 [note (C0 to B9)] [tick length] ...
 ```
 You can use sharps(#) at note (e.g. C#5), and case doesn't matter
 
 Type 'r' in note area and you get tick-length rest

## How to use autogen.c
 Simply use functions in "midiwriter.h"

## Brief midi file structure

### Header Part
 ```
 [Raw "MThd" text] [00 00 00 06]
 [midi type (2byte)] [track count (2byte)] [PPQN* (2byte)]
 ```
 PPQN stands for **P**ulses **P**er **Q**uarter **N**ote
 
 If midi type is `00 00` track count must be `00 01`
 
 If midi type is `00 01` or `00 02` track count can be `00 01` to `FF FF` (65536 track is too much!)

 Midi type other than 0, 1, 2 is invalid and media player cannot play the file
 
### Track Part
 ```
 [Raw "MTrk" text] [track length (4byte)]
 (Sequel of [delta time(1~4byte)] [midi event (variable length)]) [00 FF 2F 00]
 ```

### About Delta Time
 Delta time is time system that midi file uses
 |---------------|-----------|----------------|----|
 |Ticks (Decimal)|Ticks (Hex)|Delta Time (Hex)|Info|
 |0|00|00|Minimum value|
 |64|40|40||
 |127|7F|7F|1-byte limit|
 |128|80|81 00||
 |192|C0|81 40||
 |256|01 00|82 00||
 |16383|3F FF|FF 7F|2-byte limit|
 |16384|40 00|81 00 00||
 |2097151|1F FF FF|FF FF 7F|3-byte limit|
 |2097152|20 00 00|81 00 00 00||
 |268435455|0F FF FF FF|FF FF FF 7F|Maximum value|
 
 Conversion folmula is as following:
 ```
 Binary value 0000aaaa aaabbbbb bbcccccc cddddddd is:
              1aaaaaaa 1bbbbbbb 1ccccccc 0ddddddd
			  (when 'aaaaaaa' is non-zero)
 ```
 