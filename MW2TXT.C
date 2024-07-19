/*Martin Walker (GB/GG) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
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

int sysMode = 2;

int drvVers = 1;

const unsigned char MagicBytes[12] = { 0x3E, 0x77, 0xE0, 0x24, 0x3E, 0xFF, 0xE0, 0x25, 0x3E, 0x8F, 0xE0, 0x26 };
const unsigned char MagicBytesGGA[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xC5, 0x3E, 0x0F, 0x32, 0x03 };
const unsigned char MagicBytesGGB[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xDE, 0x3E, 0x0F, 0x32, 0x03 };
const unsigned char MagicBytesGGC[9] = { 0x3E, 0x0D, 0x32, 0x04, 0xC7, 0x3E, 0x0F, 0x32, 0x03 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptrs[4], long nextPtr);
void getSeqList(unsigned long list[], long offset1, long offset2);
void seqs2txt(unsigned long list[], long endPtr);
void getSeqListOld(unsigned long list[], long offset1, long offset2);
void seqs2txtOld(unsigned long list[], long endPtr, int songNum);
void nextSong();

unsigned long seqList[500];
unsigned long chanPts[3];
unsigned static char* romData;
int totalSeqs;

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

int main(int args, char* argv[])
{
	printf("Martin Walker (GB/GG) to TXT converter\n");
	if (args < 3 || args > 4)
	{
		printf("Usage: MW2TXT <rom> <bank> <format>\n");
		printf("Format: 1 = GB (default), 2 = GG\n");
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
			if (strtol(argv[3], NULL, 16) == 1)
			{
				sysMode = 1;
				printf("Selected format: Game Boy\n");
			}
			else if (strtol(argv[3], NULL, 16) == 2)
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
				/*Game Boy*/
				if (sysMode == 1)
				{
					bankAmt = bankSize;
				}
				/*Game Gear*/
				else if (sysMode == 2)
				{
					if (bank % 2 == 0)
					{
						bankAmt = bankSize;
					}
					else
					{
						bankAmt = bankSize * 2;
					}

				}
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
					song2txt(songNum, songInfo, nextPtr);
					i += 8;
					songNum++;
				}
				getSeqList(seqList, seqPtrList, seqData);
				seqs2txt(seqList, songList);
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
				song2txt(songNum, songInfo, nextPtr);
				getSeqListOld(seqList, seqPtrList, seqPtrList2);
				seqs2txtOld(seqList, songList, songNum);
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

/*Convert song data to TXT*/
void song2txt(int songNum, long ptrs[4], long nextPtr)
{
	long curPos = 0;
	int index = 0;
	int curSeq = 0;
	int repeat = 0;
	int curChan = 0;
	long pattern[2];
	int endSong = 0;

	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{
		for (curChan = 0; curChan < 3; curChan++)
		{
			fprintf(txt, "Channel %i:\n", curChan + 1);
			index = ptrs[curChan] - bankAmt;
			if (curChan == 0)
			{
				endSong = 0;
				while ((index + bankAmt) != ptrs[1] && endSong == 0)
				{
					pattern[0] = romData[index];
					pattern[1] = romData[index + 1];
					if (pattern[0] == 0xFF)
					{
						fprintf(txt, "End of song with loop\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xFE)
					{
						fprintf(txt, "End of song, no loop\n");
						endSong = 1;
					}
					else if (pattern[0] >= 0x40 && pattern[0] <= 0x49)
					{
						repeat = pattern[0] - 0x40;
						fprintf(txt, "Set repeat value: %i\n", repeat);
						index++;
					}
					else
					{
						fprintf(txt, "Sequence: %i, times played: %i\n", pattern[1], pattern[0]);
						index += 2;
					}
				}
			}
			else if (curChan == 1)
			{
				endSong = 0;
				while ((index + bankAmt) != ptrs[2] && endSong == 0)
				{
					pattern[0] = romData[index];
					pattern[1] = romData[index + 1];
					if (pattern[0] == 0xFF && pattern[1] == 0x00)
					{
						fprintf(txt, "End of song with loop\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xFE)
					{
						fprintf(txt, "End of song, no loop\n");
						endSong = 1;
					}
					else if (pattern[0] >= 0x40 && pattern[0] <= 0x50)
					{
						repeat = pattern[0] - 0x40;
						fprintf(txt, "Set repeat value: %i\n", repeat);
						index++;
					}
					else
					{
						fprintf(txt, "Sequence: %i, times played: %i\n", pattern[1], pattern[0]);
						index += 2;
					}
				}
			}
			else if (curChan == 2)
			{
				endSong = 0;
				while (((index + bankAmt) != nextPtr && endSong == 0) || ((index + bankAmt) != seqData && endSong == 0))
				{
					pattern[0] = romData[index];
					pattern[1] = romData[index + 1];
					if (pattern[0] == 0xFF)
					{
						fprintf(txt, "End of song with loop\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xFE)
					{
						fprintf(txt, "End of song, no loop\n");
						endSong = 1;
					}
					else if (pattern[0] >= 0x40 && pattern[0] <= 0x49)
					{
						repeat = pattern[0] - 0x40;
						fprintf(txt, "Set repeat value: %i\n", repeat);
						index++;
					}
					else
					{
						fprintf(txt, "Sequence: %i, times played: %i\n", pattern[1], pattern[0]);
						index += 2;
					}
				}
			}
			fprintf(txt, "\n");
		}
		fclose(txt);
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

/*Convert sequence data to TXT*/
void seqs2txt(unsigned long list[], long endPtr)
{
	int songEnd = 0;
	sprintf(outfile, "seqs.txt");
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{

		unsigned char command[2];
		int seqNum = 0;
		int curPos = 0;
		int seqPos = 0;
		int curNote = 0;
		int k = 0;
		int seqsLeft = totalSeqs;
		long newEnd = endPtr - bankAmt;
		signed int transpose = 0;
		curPos = list[0];
		while (curPos < newEnd)
		{
			for (k = 0; k < totalSeqs; k++)
			{
				if (curPos == list[k])
				{
					if (k != 0)
					{
						fprintf(txt, "\n");
					}
					fprintf(txt, "Sequence %i:\n", k);
				}
			}

			command[0] = romData[curPos];
			command[1] = romData[curPos + 1];

			if (command[0] == 0x80)
			{
				fprintf(txt, "Change instrument: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x81)
			{
				fprintf(txt, "Rest: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x82)
			{
				fprintf(txt, "Turn on portamento with amount: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x83)
			{
				fprintf(txt, "Set tempo: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x84)
			{
				fprintf(txt, "Set effect type?: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x85)
			{
				fprintf(txt, "Change note length type: %i\n", command[1]);
				curPos += 2;
			}
			else if (command[0] == 0x86)
			{
				transpose = (signed char)command[1];
				fprintf(txt, "Set transpose: %i\n", transpose);
				curPos += 2;
			}
			else if (command[0] == 0xFF)
			{
				fprintf(txt, "End of sequence\n");
				curPos++;
			}
			else
			{
				fprintf(txt, "Note: %i, length: %i\n", command[0], command[1]);
				curPos += 2;
			}
		}
		fclose(txt);
	}
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
	while ((k+n) != l)
	{
		if (romData[k + n] != 0xFF)
		{
			addAmt = romData[l+n] * 256;
			list[cnt] = romData[k+n] + addAmt + seqData - bankAmt;
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

/*Convert sequence data to TXT (Speedball 2)*/
void seqs2txtOld(unsigned long list[], long endPtr, int songNum)
{
	int songEnd = 0;
	sprintf(outfile, "seqs%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{
		unsigned char command[2];
		int seqNum = 0;
		int curPos = 0;
		int seqPos = 0;
		int curNote = 0;
		int curLen = 0;
		int curInst = 0;
		int k = 0;
		int l = 0;
		int seqsLeft = totalSeqs;
		long newEnd = endPtr - bankAmt;
		signed int transpose = 0;
		curPos = list[0];
		while (curPos < newEnd)
		{
			for (k = 0; k <= totalSeqs; k++)
			{
				if (curPos == list[k])
				{
					if (k != 0)
					{
						fprintf(txt, "\n");
					}
					fprintf(txt, "Sequence %i:\n", k);
					for (l = 0; l < 4; l++)
					{
						if (chanPts[l] == k)
						{
							fprintf(txt, "Channel %i start\n", l + 1);
						}
					}
				}
			}
			command[0] = romData[curPos];
			command[1] = romData[curPos + 1];

			if (command[0] >= 0x80 && command[0] < 0xC0)
			{
				curLen = command[0] - 0x80;
				fprintf(txt, "Change note length: %i\n", curLen);
				curPos++;
			}
			else if (command[0] == 0xFF)
			{
				fprintf(txt, "End of sequence\n");
				curPos++;
			}
			else
			{
				curNote = command[1];
				curInst = command[0];
				fprintf(txt, "Note: %i, instrument: %i\n", command[1], command[0]);
				curPos += 2;
			}
		}
		fclose(txt);
	}
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
	song2txt(songNum, songInfo, nextPtr);
	getSeqListOld(seqList, seqPtrList, seqPtrList2);
	seqs2txtOld(seqList, songList, songNum);
}