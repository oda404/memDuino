// Copyright (c) Olaru Alexandru <olarualexandru404@gmail.com>
// Licensed under the MIT license found in the LICENSE file in the root of this repository.

#include<stdio.h>
#include<string.h>
#include<math.h>

#include"serialW.h"

#define MAX_INPUT_LEN 64

int asciiZero = (int)'0', asciiNine = (int)'9';

int parseIntFromStr(const char *str)
{
	int integer = 0;

	for(int i = 0; i < strlen(str); ++i)
		if((int)str[i] >= asciiZero && (int)str[i] <= asciiNine)
			integer = integer * 10 + (int)str[i] - asciiZero;

	return integer;
}

int getUsedMemInMB(const int *memTotal, const int *memFree, const int *buffers, const int *cached, const int *sReclaimable, const int *shmem)
{
	// taken from https://stackoverflow.com/questions/41224738/how-to-calculate-system-memory-usage-from-proc-meminfo-like-htop/41251290#41251290
	return ((*memTotal - *memFree) - (*buffers + (*cached + *sReclaimable - *shmem))) / 1024;
}

void createPacket(int integer, int integerLen, char *destStr)
{
	int aux = 1;
	int i = 0;
	
	destStr[i++] = 'S';

	while(aux <= integerLen)
	{
		destStr[i++] = (char)(integer / (int)pow(10, integerLen - aux) + asciiZero);
		integer = integer % (int)pow(10, integerLen - aux++);
	}

	destStr[i++] = 'E';
	
	destStr[i++] = '\0';

	/* start and end the packet with S and E respectively */
}

int getIntLen(int integer)
{
	return (int)log10(integer) + 1;
}

int main(void)
{
#ifdef __linux__
	FILE *file;

	serialInit("ttyUSB0");

	int memTotal, 
	buffers, 
	cached, 
	memFree, 
	shmem, 
	sReclaimable,
	usedMem,
	usedMemLen;

	char line[MAX_INPUT_LEN];

	while(1)
	{
		if(!(file = fopen("/proc/meminfo", "r")))
		{
			printf("Fatal error: meminfo was not found in /proc/\n");
			break;
		}

		while(fgets(line, MAX_INPUT_LEN, file))
		{
			if(strstr(line, "MemTotal:"))
				memTotal = parseIntFromStr(line);
			else if(strstr(line, "Buffers:"))
				buffers = parseIntFromStr(line);
			else if(strstr(line, "Cached:") && !strstr(line, "SwapCached:"))
				cached = parseIntFromStr(line);
			else if(strstr(line, "MemFree:"))
				memFree = parseIntFromStr(line);
			else if(strstr(line, "Shmem:"))
				shmem = parseIntFromStr(line);
			else if(strstr(line, "SReclaimable:"))
				sReclaimable = parseIntFromStr(line);
		}

		usedMem = getUsedMemInMB(&memTotal, &memFree, &buffers, &cached, &sReclaimable, &shmem);
		usedMemLen = getIntLen(usedMem);
		char usedMemStr[usedMemLen + 3];
		createPacket(usedMem, usedMemLen, usedMemStr);

		writeToSerial(usedMemStr);

		fclose(file);

		sleep(1);
	}

	serialClose();

#elif _WIN32

#define DIV 1048576

	serialInit("COM3");

	/* Hide console on startup by default */
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);


	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	int usedMem,
		usedMemLen;

	while (1)
	{
		GlobalMemoryStatusEx(&statex);

		/* Calculate the used memory in MB from the used memory percentage */
		usedMem = (float)statex.dwMemoryLoad / 100.0f * (float)(statex.ullTotalPhys / DIV);

		usedMemLen = getIntLen(usedMem);
		char *usedMemStr = malloc(usedMemLen + 3);
		createPacket(usedMem, usedMemLen, usedMemStr);

		writeToSerial(usedMemStr);
		
		free(usedMemStr);

		Sleep(1000);
	}

	serialClose();

#endif

	return 0;
}
