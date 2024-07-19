/*Martin Walker (GB/GG) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
long bank;
long offset;
long addTable;
long baseValue;
long tablePtrLoc;
long seqPtrList;
long seqPtrList2;
long seqData;
long songList;
long patData;
int i, j;
char outfile[1000000];
int songNum;
long songInfo[4];
long curSpeed;
long nextPtr;
long endPtr;
long bankAmt;
long nextBase;
int addValues[7];

long switchPoint[400][2];
int switchNum = 0;

int sysMode = 2;

int drvVers = 1;

char *argv3;

const unsigned char MagicBytes[12] = { 0x3E, 0x77, 0xE0, 0x24, 0x3E, 0xFF, 0xE0, 0x25, 0x3E, 0x8F, 0xE0, 0x26 };
const unsigned char MagicBytesGGA[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xC5, 0x3E, 0x0F, 0x32, 0x03 };
const unsigned char MagicBytesGGB[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xDE, 0x3E, 0x0F, 0x32, 0x03 };
const unsigned char MagicBytesGGC[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xC7, 0x3E, 0x0F, 0x32, 0x03 };
const unsigned char MagicBytesGGD[9] = { 0x3E, 0x0D, 0x32, 0xF4, 0xDE, 0x3E, 0x0F, 0x32, 0xF3 };

unsigned long seqList[500];
unsigned long chanPts[3];
int totalSeqs;

unsigned static char* romData;
unsigned static char* midData;
unsigned static char* ctrlMidData;

long midLength;

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst);
int WriteDeltaTime(unsigned char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptrs[4], long nextPtr);
void song2midOld(int songNum, long ptrs[4], long nextPtr);
void getSeqList(unsigned long list[], long offset1, long offset2);
void getSeqListOld(unsigned long list[], long offset1, long offset2);
void nextSong();

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

unsigned int WriteNoteEvent(unsigned char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;

	if (firstNote == 1)
	{
		if (curChan != 3)
		{
			Write8B(&buffer[pos], 0xC0 | curChan);
		}
		else
		{
			Write8B(&buffer[pos], 0xC9);
		}

		Write8B(&buffer[pos + 1], inst);
		Write8B(&buffer[pos + 2], 0);

		if (curChan != 3)
		{
			Write8B(&buffer[pos + 3], 0x90 | curChan);
		}
		else
		{
			Write8B(&buffer[pos + 3], 0x99);
		}

		pos += 4;
	}

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 100);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;

}

int WriteDeltaTime(unsigned char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
}

int main(int args, char* argv[])
{
	printf("Martin Walker (GB/GG) to MIDI converter\n");
	if (args < 3 || args > 4)
	{
		printf("Usage: MW2MID <rom> <bank> <format>\n");
		printf("Format: gb, gg\n");
		return -1;
	}
	else
	{
		if (args == 3)
		{
			sysMode = 1;
		}
		else if (args == 4)
		{
			argv3 = argv[3];
			if (strcmp(argv3, "gb") == 0 || strcmp(argv3, "GB") == 0)
			{
				sysMode = 1;
				printf("Selected format: Game Boy\n");
			}
			else if (strcmp(argv3, "gg") == 0 || strcmp(argv3, "GG") == 0)
			{
				sysMode = 2;
				printf("Selected format: Game Gear\n");
			}
			else
			{
				printf("ERROR: Invalid mode switch!\n");
				exit(1);
			}
		}
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
		}
		fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
		romData = (unsigned char*)malloc(bankSize);
		fread(romData, 1, bankSize, rom);
		fclose(rom);

				if (sysMode == 2)
				{
					if (ReadLE16(&romData[1]) >= (bankSize * 2))
					{
						bankAmt = bankSize * 2;
					}

				}

		/*Try to search the bank for base table*/
		for (i = 0; i < bankSize; i++)
		{
			if (!memcmp(&romData[i], MagicBytes, 12))
			{
				if ((romData[i + 13]) == 0x21)
				{
					/*Standard driver version*/
					drvVers = 1;
				}
				else if ((romData[i + 13]) == 0xCB)
				{
					/*Early driver - Speedball 2*/
					drvVers = 0;
				}

				tablePtrLoc = bankAmt + i + 13;
				printf("Found base table values at at address 0x%04x!\n", tablePtrLoc);
				if (drvVers == 0)
				{
					printf("Detected old driver version (Speedball 2: Brutal Deluxe).\n");
				}

				if (drvVers == 1)
				{
					addTable = ReadLE16(&romData[tablePtrLoc + 1 - bankAmt]);
					baseValue = ReadLE16(&romData[tablePtrLoc + 4 - bankAmt]);
				}
				else if (drvVers == 0)
				{
					baseValue = ReadLE16(&romData[tablePtrLoc + 3 - bankAmt]);
					addTable = ReadLE16(&romData[tablePtrLoc + 6 - bankAmt]);
				}

				printf("Base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);
				break;
			}
		}

		if (sysMode == 2)
		{
			/*Try to search the bank for base table*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], MagicBytesGGA, 9))
				{
					tablePtrLoc = bankAmt + i + 10;
					printf("Found base table values at at address 0x%04x!\n", tablePtrLoc);
					addTable = ReadLE16(&romData[tablePtrLoc + 5 - bankAmt]);

					baseValue = ReadLE16(&romData[tablePtrLoc + 8 - bankAmt]);

					if (baseValue == 0x21C9)
					{
						addTable = ReadLE16(&romData[tablePtrLoc + 10 - bankAmt]);
						baseValue = ReadLE16(&romData[tablePtrLoc + 13 - bankAmt]);
					}
					
					printf("Base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);
					break;
				}
			}

			/*Alternate method*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], MagicBytesGGB, 9))
				{
					tablePtrLoc = bankAmt + i + 10;
					printf("Found base table values at at address 0x%04x!\n", tablePtrLoc);
					addTable = ReadLE16(&romData[tablePtrLoc + 5 - bankAmt]);

					baseValue = ReadLE16(&romData[tablePtrLoc + 8 - bankAmt]);

					if (baseValue == 0x21C9)
					{
						addTable = ReadLE16(&romData[tablePtrLoc + 10 - bankAmt]);
						baseValue = ReadLE16(&romData[tablePtrLoc + 13 - bankAmt]);
					}

					printf("Base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);
					break;
				}
			}

			/*Alternate method*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], MagicBytesGGC, 9))
				{
					tablePtrLoc = bankAmt + i + 10;
					printf("Found base table values at at address 0x%04x!\n", tablePtrLoc);
					addTable = ReadLE16(&romData[tablePtrLoc + 5 - bankAmt]);

					baseValue = ReadLE16(&romData[tablePtrLoc + 8 - bankAmt]);

					if (baseValue == 0x21C9)
					{
						addTable = ReadLE16(&romData[tablePtrLoc + 10 - bankAmt]);
						baseValue = ReadLE16(&romData[tablePtrLoc + 13 - bankAmt]);
					}

					printf("Base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);
					break;
				}
			}

			/*Alternate method*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], MagicBytesGGD, 9))
				{
					tablePtrLoc = bankAmt + i + 10;
					printf("Found base table values at at address 0x%04x!\n", tablePtrLoc);
					addTable = ReadLE16(&romData[tablePtrLoc + 5 - bankAmt]);

					baseValue = ReadLE16(&romData[tablePtrLoc + 8 - bankAmt]);
					printf("Base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);
					break;
				}
			}
		}

		/*Get pointers to sequence and song data*/
		if (addTable != 0 && baseValue != 0)
		{
			if (drvVers == 1)
			{
				for (j = 0; j < 7; j++)
				{
					addValues[j] = ReadLE16(&romData[(addTable - bankAmt) + (2 * j)]);
				}
				seqPtrList = baseValue + addValues[0] + addValues[1] + addValues[2];
				printf("Sequence data pointer list: 0x%04x\n", seqPtrList);
				seqData = seqPtrList + addValues[3];
				printf("Sequence data: 0x%04x\n", seqData);
				songList = seqData + addValues[4];
				printf("Song list: 0x%04x\n", songList);
				patData = songList + addValues[5];
				printf("Pattern data: 0x%04x\n", patData);

				i = songList;
				songNum = 1;
				getSeqList(seqList, seqPtrList, seqData);
				/*Get song information*/
				while (i != patData)
				{
					songInfo[0] = ReadLE16(&romData[i - bankAmt]) + patData;
					printf("Song %i channel 1 pointer: 0x%04X\n", songNum, songInfo[0]);
					songInfo[1] = ReadLE16(&romData[i + 2 - bankAmt]) + patData;
					printf("Song %i channel 2 pointer: 0x%04X\n", songNum, songInfo[1]);
					songInfo[2] = ReadLE16(&romData[i + 4 - bankAmt]) + patData;
					printf("Song %i channel 3 pointer: 0x%04X\n", songNum, songInfo[2]);
					songInfo[3] = ReadLE16(&romData[i + 6 - bankAmt]);
					nextPtr = ReadLE16(&romData[i + 8 - bankAmt]) + patData;
					song2mid(songNum, songInfo, nextPtr);
					i += 8;
					songNum++;
				}
			}
			else if (drvVers == 0)
			{
				for (j = 0; j < 6; j++)
				{
					addValues[j] = ReadLE16(&romData[(addTable - bankAmt) + (2 * j)]);
				}
				seqPtrList = baseValue + addValues[1] + addValues[1];
				printf("Sequence data pointer list 1: 0x%04x\n", seqPtrList);
				seqPtrList2 = seqPtrList + addValues[2];
				printf("Sequence data pointer list 2: 0x%04x\n", seqPtrList2);
				seqData = seqPtrList + addValues[2] + addValues[2];
				printf("Sequence data: 0x%04x\n", seqData);
				songList = seqData + addValues[3];
				printf("Song list: 0x%04x\n", songList);
				patData = songList + addValues[4];
				printf("Pattern data: 0x%04x\n", patData);

				i = songList;
				songNum = 1;
				getSeqListOld(seqList, seqPtrList, seqPtrList2);
				/*Get song information*/
				songInfo[0] = patData + romData[i - bankAmt];
				printf("Song %i channel 1 pointer: 0x%04X\n", songNum, songInfo[0]);
				songInfo[1] = patData + romData[i + 1 - bankAmt];
				printf("Song %i channel 2 pointer: 0x%04X\n", songNum, songInfo[1]);
				songInfo[2] = patData + romData[i + 2 - bankAmt];
				printf("Song %i channel 3 pointer: 0x%04X\n", songNum, songInfo[2]);
				if (songNum == 1)
				{
					nextPtr = patData + romData[i + 16];
				}
				else if (songNum == 2)
				{
					nextPtr == 0x7FFF;
				}
				song2midOld(songNum, songInfo, nextPtr);
				nextSong();
			}
		}
		else
		{
			printf("ERROR: Magic bytes not found!\n");
			exit(-1);
		}
		printf("The operation was successfully completed!\n");
	}
}

/*Convert song data to MIDI*/
void song2mid(int songNum, long ptrs[4], long nextPtr)
{
	static const char* TRK_NAMES[3] = { "Square 1", "Square 2", "Wave"};
	long romPos = 0;
	unsigned int midPos = 0;
	int trackCnt = 3;
	int curTrack = 0;
	int curSeq = 0;
	long midTrackBase = 0;
	unsigned int curDelay = 0;
	int midChan = 0;
	int seqEnd = 0;
	int trackEnd = 0;
	int noteTrans = 0;
	int ticks = 120;
	int k = 0;
	long startPos = 0;

	unsigned int ctrlMidPos = 0;
	long ctrlMidTrackBase = 0;

	int valSize = 0;

	long trackSize = 0;

	unsigned int curNote = 0;
	int curNoteLen = 0;
	int lastNote = 0;

	int tempByte = 0;
	long tempPos = 0;

	long tempo = 160;

	int curInst = 0;

	int macRepeat = 0;
	long macStart = 0;
	long macEnd = 0;

	int playTimes = 1;

	unsigned long patPos = 0;
	unsigned long seqPos = 0;

	unsigned char command[3];

	signed int transpose = 0;

	int firstNote = 1;

	int timeVal = 0;

	int holdNote = 0;

	long ctrlDelay = 0;
	long masterDelay = 0;

	midPos = 0;
	ctrlMidPos = 0;
	switchNum = 0;

	for (switchNum = 0; switchNum < 400; switchNum++)
	{
		switchPoint[switchNum][0] = -1;
		switchPoint[switchNum][1] = 0;
	}

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;

		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		for (curTrack = 0; curTrack < trackCnt; curTrack++)
		{
			transpose = 0;
			firstNote = 1;
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			midPos += 8;
			midTrackBase = midPos;

			curDelay = 0;
			ctrlDelay = 0;
			trackEnd = 0;

			curNote = 0;
			lastNote = 0;
			curNoteLen = 0;

			masterDelay = 0;

			switchNum = 0;

			/*Add track header*/
			valSize = WriteDeltaTime(midData, midPos, 0);
			midPos += valSize;
			WriteBE16(&midData[midPos], 0xFF03);
			midPos += 2;
			Write8B(&midData[midPos], strlen(TRK_NAMES[curTrack]));
			midPos++;
			sprintf((char*)&midData[midPos], TRK_NAMES[curTrack]);
			midPos += strlen(TRK_NAMES[curTrack]);

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);
			patPos = ptrs[curTrack] - bankAmt;
			if (ptrs[curTrack] == 0)
			{
				trackEnd = 1;
			}

			while (trackEnd == 0)
			{
				/*Repeat macro*/
				if (romData[patPos] > 0x40 && romData[patPos] <= 0x50)
				{
					macRepeat = romData[patPos] - 0x40;
					macStart = patPos + 1;
					patPos = macStart;
				}

				/*Get current sequence and repeat times*/
				if (romData[patPos] != 0xFF && romData[patPos] != 0xFE && romData[patPos] < 0x40)
				{
					playTimes = romData[patPos];
					curSeq = romData[patPos + 1];
					seqEnd = 0;
					patPos += 2;
				}
				/*End of repeat macro*/
				else if (romData[patPos] == 0x40)
				{
					if (macRepeat > 1)
					{
						patPos = macStart;
						macRepeat--;
					}
					else
					{
						patPos++;
					}

				}

				/*End of track*/
				else if (romData[patPos] == 0xFF || romData[patPos] == 0xFE || romData[patPos] == 0xFD)
				{
					trackEnd = 1;
				}

				/*Go to the current sequence*/
				seqPos = seqList[curSeq];
				startPos = seqPos;
				while (seqEnd == 0)
				{
					command[0] = romData[seqPos];
					command[1] = romData[seqPos + 1];
					command[2] = romData[seqPos + 2];

					if (curTrack != 0)
					{
						for (switchNum = 0; switchNum < 90; switchNum++)
						{
							if (switchPoint[switchNum][0] == masterDelay)
							{
								transpose = switchPoint[switchNum][1];
							}
						}
					}

					/*Change instrument*/
					if (command[0] == 0x80)
					{
						curInst = command[1];
						firstNote = 1;
						seqPos += 2;
					}
					/*Rest*/
					else if (command[0] == 0x81)
					{
						curDelay += (command[1] * 10);
						ctrlDelay += (command[1] * 10);
						masterDelay += (command[1] * 10);
						seqPos += 2;
					}
					/*Turn on portamento*/
					else if (command[0] == 0x82)
					{
						tempPos = WriteDeltaTime(midData, midPos, curDelay);
						midPos += tempPos;
						Write8B(&midData[midPos], (0xE0 | curTrack));
						Write8B(&midData[midPos + 1], 0);
						Write8B(&midData[midPos + 2], 0x40);
						Write8B(&midData[midPos + 3], 0);
						curDelay = 0;
						firstNote = 1;
						midPos += 3;
						seqPos += 2;
					}
					/*Set tempo*/
					else if (command[0] == 0x83)
					{
						if ((command[1] * 1.2) != tempo)
						{
							ctrlMidPos++;
							valSize = WriteDeltaTime(ctrlMidData, ctrlMidPos, ctrlDelay);
							ctrlDelay = 0;
							ctrlMidPos += valSize;
							WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5103);
							ctrlMidPos += 3;
							tempo = command[1] * 1.2;
							WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
							ctrlMidPos += 2;
						}
						seqPos += 2;

					}
					/*Set effect type?*/
					else if (command[0] == 0x84)
					{
						seqPos += 2;
					}
					/*Change note length type*/
					else if (command[0] == 0x85)
					{
						seqPos += 2;
					}
					/*Set transpose*/
					else if (command[0] == 0x86)
					{
						transpose = (signed char)command[1];
						if (curTrack == 0)
						{
							switchPoint[switchNum][0] = masterDelay;
							switchPoint[switchNum][1] = transpose;
							switchNum++;
						}
						seqPos += 2;
					}
					/*End of sequence*/
					else if (command[0] == 0xFF)
					{
						if (playTimes > 1)
						{
							seqPos = startPos;
							playTimes--;
						}
						else
						{
							seqEnd = 1;
						}
					}
					else
					{
						curNote = command[0] + transpose;
						if (curNote > 0x80)
						{
							curNote -= 0x80;
						}
						curNoteLen = command[1] * 10;
						ctrlDelay += curNoteLen;
						masterDelay += curNoteLen;
						tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack, curInst);
						firstNote = 0;
						midPos = tempPos;
						curDelay = 0;
						seqPos += 2;
					}
				}
			}

			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

		}
		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);
		
		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		fclose(mid);
	}
}

/*Convert song data to MIDI (Speedball 2)*/
void song2midOld(int songNum, long ptrs[4], long nextPtr)
{
	static const char* TRK_NAMES[3] = { "Square 1", "Square 2", "Wave"};
	long romPos = 0;
	unsigned int midPos = 0;
	int trackCnt = 3;
	int curTrack = 0;
	int curSeq = 0;
	long midTrackBase = 0;
	unsigned int curDelay = 0;
	int midChan = 0;
	int seqEnd = 0;
	int trackEnd = 0;
	int noteTrans = 0;
	int ticks = 120;
	int k = 0;
	long startPos = 0;

	unsigned int ctrlMidPos = 0;
	long ctrlMidTrackBase = 0;

	int valSize = 0;

	long trackSize = 0;

	unsigned int curNote = 0;
	int curNoteLen = 0;
	int lastNote = 0;

	int tempByte = 0;
	long tempPos = 0;

	long tempo = 120;

	int curInst = 0;

	int macRepeat = 0;
	long macStart = 0;
	long macEnd = 0;

	int playTimes = 1;

	unsigned long patPos = 0;
	unsigned long seqPos = 0;

	unsigned char command[3];

	signed int transpose = 0;

	int firstNote = 1;

	int timeVal = 0;

	int holdNote = 0;

	long ctrlDelay = 0;
	long masterDelay = 0;

	midPos = 0;
	ctrlMidPos = 0;
	switchNum = 0;

	for (switchNum = 0; switchNum < 400; switchNum++)
	{
		switchPoint[switchNum][0] = -1;
		switchPoint[switchNum][1] = 0;
	}

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;

		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		for (curTrack = 0; curTrack < trackCnt; curTrack++)
		{
			transpose = 0;
			firstNote = 1;
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			midPos += 8;
			midTrackBase = midPos;

			curDelay = 0;
			ctrlDelay = 0;
			trackEnd = 0;

			curNote = 0;
			lastNote = 0;
			curNoteLen = 0;

			masterDelay = 0;

			switchNum = 0;

			/*Add track header*/
			valSize = WriteDeltaTime(midData, midPos, 0);
			midPos += valSize;
			WriteBE16(&midData[midPos], 0xFF03);
			midPos += 2;
			Write8B(&midData[midPos], strlen(TRK_NAMES[curTrack]));
			midPos++;
			sprintf((char*)&midData[midPos], TRK_NAMES[curTrack]);
			midPos += strlen(TRK_NAMES[curTrack]);

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);
			patPos = ptrs[curTrack] - bankAmt;
			if (ptrs[curTrack] == 0)
			{
				trackEnd = 1;
			}

			while (trackEnd == 0)
			{
				/*Repeat macro*/
				if (romData[patPos] > 0x40 && romData[patPos] <= 0x50)
				{
					macRepeat = romData[patPos] - 0x40;
					macStart = patPos + 1;
					patPos = macStart;
				}

				/*Get current sequence and repeat times*/
				if (romData[patPos] != 0xFF && romData[patPos] != 0xFE && romData[patPos] < 0x40)
				{
					playTimes = romData[patPos];
					
					curSeq = romData[patPos + 1];
					if (curTrack == 1 && songNum == 1)
					{
						curSeq -= 1;
					}
					else if (curTrack == 2 && songNum == 1)
					{
						if (curSeq != 0)
						{
							curSeq -= 1;
						}
					}
					seqEnd = 0;
					patPos += 2;
				}
				/*End of repeat macro*/
				else if (romData[patPos] == 0x40)
				{
					if (macRepeat > 1)
					{
						patPos = macStart;
						macRepeat--;
					}
					else
					{
						patPos++;
					}

				}

				/*End of track*/
				else if (romData[patPos] == 0xFF || romData[patPos] == 0xFE || romData[patPos] == 0xFD)
				{
					trackEnd = 1;
				}

				/*Go to the current sequence*/
				seqPos = seqList[curSeq];
				startPos = seqPos;
				while (seqEnd == 0)
				{
					command[0] = romData[seqPos];
					command[1] = romData[seqPos + 1];
					command[2] = romData[seqPos + 2];

					/*Set note length*/
					if (command[0] >= 0x80 && command[0] <= 0xB0)
					{
						curNoteLen = (command[0] - 0x80) * 10;
						seqPos++;
					}

					/*End of sequence*/
					else if (command[0] == 0xFF)
					{
						if (playTimes > 1)
						{
							seqPos = startPos;
							playTimes--;
						}
						else
						{
							seqEnd = 1;
						}
					}
					else
					{
						curNote = command[1];
						if (curNote > 0x80)
						{
							curNote -= 0x80;
						}
						ctrlDelay += curNoteLen;
						masterDelay += curNoteLen;
						if (curInst != command[0])
						{
							curInst = command[0];
							if (curInst >= 0x80)
							{
								curInst = 0x7F;
							}
							firstNote = 1;
						}				
						tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack, curInst);
						firstNote = 0;
						midPos = tempPos;
						curDelay = 0;
						seqPos += 2;
					}
				}
			}

			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

		}
		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);
		
		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		fclose(mid);
	}
}

/*Get pointers to each sequence*/
void getSeqList(unsigned long list[], long offset1, long offset2)
{
	int cnt = 0;
	long offset3 = offset1 - bankAmt;
	long offset4 = offset2 - bankAmt;
	int k = offset3;

	while (k != offset4)
	{
		list[cnt] = offset4 + ReadLE16(&romData[k]);
		k += 2;
		cnt++;
	}
	totalSeqs = cnt;


}

/*Get pointers to each sequence (Speedball 2)*/
void getSeqListOld(unsigned long list[], long offset1, long offset2)
{
	int cnt = 0;
	int cnt2 = 0;
	int chanNum = 0;
	long offset3 = offset1 - bankAmt;
	long offset4 = offset2 - bankAmt;
	int k = offset3;
	int l = offset4;
	int n = 0;
	int addAmt = 0;
	int addAmt2 = 0;
	int start = offset4;
	for (n = 0; n < 3; n++)
	{
		chanPts[n] = 0;
	}
	n = 0;
	while ((k + n) != l)
	{
		if (romData[k + n] != 0xFF)
		{
			addAmt = romData[l + n] * 256;
			list[cnt] = romData[k + n] + addAmt + seqData - bankAmt;
			/*printf("Sequence %i: 0x%04X\n", cnt, (list[cnt] + bankAmt));*/
			cnt++;
		}
		else if (romData[k + n] == 0xFF)
		{
			if (n != 0)
			{
				chanNum++;
				chanPts[chanNum] = cnt + 1;
			}
		}
		n++;
	}
	totalSeqs = cnt;

}

void nextSong()
{
	/*Try to search the bank for NEXT base table*/
	i = tablePtrLoc;
	baseValue = ReadLE16(&romData[i + 11 - bankAmt]);
	addTable = ReadLE16(&romData[i + 14 - bankAmt]);
	printf("Next song base value: 0x%04x\nAdd table: 0x%04x\n", baseValue, addTable);

	for (j = 0; j < 6; j++)
	{
		addValues[j] = ReadLE16(&romData[(addTable - bankAmt) + (2 * j)]);
	}
	seqPtrList = baseValue + addValues[0] + addValues[1] + addValues[1];
	printf("Sequence data pointer list 1: 0x%04x\n", seqPtrList);
	seqPtrList2 = seqPtrList + addValues[2];
	printf("Sequence data pointer list 2: 0x%04x\n", seqPtrList2);
	seqData = seqPtrList + addValues[2] + addValues[2];
	printf("Sequence data: 0x%04x\n", seqData);
	songList = seqData + addValues[3];
	printf("Song list: 0x%04x\n", songList);
	patData = songList + addValues[4];
	printf("Pattern data: 0x%04x\n", patData);

	i = songList;
	songNum = 2;
	/*Get song information*/
	songInfo[0] = patData + romData[i - bankAmt];
	printf("Song %i channel 1 pointer: 0x%04X\n", songNum, songInfo[0]);
	songInfo[1] = patData + romData[i + 1 - bankAmt];
	printf("Song %i channel 2 pointer: 0x%04X\n", songNum, songInfo[1]);
	songInfo[2] = patData + romData[i + 2 - bankAmt];
	printf("Song %i channel 3 pointer: 0x%04X\n", songNum, songInfo[2]);
	if (songNum == 1)
	{
		nextPtr = patData + romData[i + 16];
	}
	else if (songNum == 2)
	{
		nextPtr == 0x7FFF;
	}
	getSeqListOld(seqList, seqPtrList, seqPtrList2);
	song2midOld(songNum, songInfo, nextPtr);
}