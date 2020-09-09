// Copyright (c) Olaru Alexandru <xdxalexandru404@gmail.com>
// Licensed under the MIT license found in the LICENSE file in the root of this repository.

#include"memDuino.h"

#include"serial.h"

#include<math.h>

#if defined(__linux__)

#include<time.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#define BUFF_LENGTH 64

static unsigned int parse_uint_from_str(const char *str)
{
	unsigned int integer = 0;
	size_t i = 0;

	for(; i < strlen(str); ++i)
	{
		if((int)str[i] >= 48 && (int)str[i] <= 57)
		{
			integer = integer * 10 + (int)str[i] - 48;
		}
	}

	return integer;
}

static int str_starts_with(const char *targetStr, const char *subStr)
{
	size_t i = 0;

	for(; i < strlen(subStr); ++i)
	{
		if(targetStr[i] != subStr[i])
		{
			return 0;
		}
	}

	return 1;
}

static void set_used_mem_mb(MemDuino* memduino)
{
	// taken from https://stackoverflow.com/questions/41224738/how-to-calculate-system-memory-usage-from-proc-meminfo-like-htop/41251290#41251290
	memduino->mem_info.used_mem = 
	((memduino->mem_info.mem_total - memduino->mem_info.mem_free) - 
	(memduino->mem_info.buffers + 
	(memduino->mem_info.cached + memduino->mem_info.s_reclaimable - memduino->mem_info.sh_mem))) /1024;
}

#endif // __linux__

static void create_packet(char *packet, unsigned int used_mem, size_t used_mem_dig_cnt)
{
	size_t i = 0;
	
	packet[i++] = 'S';

	while(i <= used_mem_dig_cnt)
	{
		packet[i] = (char)(used_mem / (int)pow(10, used_mem_dig_cnt - i) + 48);
		used_mem = used_mem % (int)pow(10, used_mem_dig_cnt - i++);
	}

	packet[i] = 'E';
	/* start and end the packet with S and E respectively */
}

static void sleep_for_ms(time_t time)
{
#if defined(__linux__)
#if _POSIX_C_SOURCE > 199309L
	struct timespec ts;
	ts.tv_sec = time / 1000;
	ts.tv_nsec = (time % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(time * 1000);
#endif // _POSIX_C_SOURCE
#elif defined(_WIN32)
	Sleep(time);
#endif //__linux__
}

#define SLEEP_INIT_TRY_MS 1500

void start_memduino(MemDuino *memduino)
{
	unsigned int init_time = 0;

	while(serial_init(memduino) == -1)
	{
		if(init_time >= memduino->info.init_timeout_ms)
		{
			printf("Failed to init memDuino!\n");
			return;
		}
		
		sleep_for_ms(SLEEP_INIT_TRY_MS);
		init_time += SLEEP_INIT_TRY_MS;
	}

#if defined(__linux__)

	FILE *file;
	char line[BUFF_LENGTH];

	while(1)
	{
		if(!(file = fopen("/proc/meminfo", "r")))
		{
			printf("Fatal error: meminfo was not found in /proc/\n");
			break;
		}

		while(fgets(line, BUFF_LENGTH, file))
		{
			if(str_starts_with(line, "MemTotal"))
			{
				memduino->mem_info.mem_total = parse_uint_from_str(line);
			}
			else if(str_starts_with(line, "Buffers"))
			{
				memduino->mem_info.buffers = parse_uint_from_str(line);
			}
			else if(str_starts_with(line, "Cached"))
			{
				memduino->mem_info.cached = parse_uint_from_str(line);
			}
			else if(str_starts_with(line, "MemFree"))
			{
				memduino->mem_info.mem_free = parse_uint_from_str(line);
			}
			else if(str_starts_with(line, "Shmem"))
			{
				memduino->mem_info.sh_mem = parse_uint_from_str(line);
			}
			else if(str_starts_with(line, "SReclaimable"))
			{
				memduino->mem_info.s_reclaimable = parse_uint_from_str(line);
			}
		}

		fclose(file);

		set_used_mem_mb(memduino);

		size_t used_mem_dig_cnt = (int)log10(memduino->mem_info.used_mem) + 1;
		char *packet = malloc(used_mem_dig_cnt + 2);

		create_packet(packet, memduino->mem_info.used_mem, used_mem_dig_cnt);

		write_to_serial(memduino, packet);

		free(packet);
		sleep_for_ms(memduino->info.update_interval_ms);
	}

	serial_close(memduino);

#elif defined(_WIN32)

	/* Hide the console window */
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);

	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	while (1)
	{
		GlobalMemoryStatusEx(&statex);

		memduino->mem_info.used_mem = (statex.ullTotalPhys - statex.ullAvailPhys) / (1024 * 1024);

		size_t used_mem_dig_cnt = (int)log10(memduino->mem_info.used_mem) + 1;
		char* packet = malloc(used_mem_dig_cnt + 2);

		create_packet(packet, memduino->mem_info.used_mem, used_mem_dig_cnt);

		write_to_serial(memduino, packet);

		free(packet);
		sleep_for_ms(memduino->info.update_interval_ms);
	}

#endif // __linux__
}
