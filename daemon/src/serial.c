// Copyright (c) Olaru Alexandru <xdxalexandru404@gmail.com>
// Licensed under the MIT license found in the LICENSE file in the root of this repository.

#include<memduinod/serial.h>

#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<fcntl.h> 
#include<errno.h>

#define PRINTFF(fmt, ...) printf("[serial] " fmt, ##__VA_ARGS__)

/**
 * TODO: Make serial_init take in more arguments like the
 * baud rate, r/w mode ??
*/

int serial_init(const char *device)
{
	int device_fd = open(device, O_WRONLY | O_NOCTTY | O_SYNC);
	
	if(device_fd < 0)
	{
		PRINTFF("Error oppening %s: %s\n", device, strerror(errno));
		return -1;
	}

	struct termios tty;

	cfmakeraw(&tty);
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_iflag &= ~(IXOFF | IXANY);

	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 0;

	cfsetospeed(&tty, (speed_t)B9600);

	if(tcsetattr(device_fd, TCSANOW, &tty) != 0)
	{
		PRINTFF("Error from tcsetattr: %s\n", strerror(errno));
		return -1;
	}

	return device_fd;
}

size_t serial_write(int device_fd, const char *data)
{
	return serial_nwrite(device_fd, data, strlen(data));
}

size_t serial_nwrite(int device_fd, const char *data, size_t n)
{
	return write(device_fd, data, n);
}

void serial_close(const int device_fd)
{
	if(close(device_fd) == -1)
		PRINTFF("Error when closing file descriptor %d: %s\n", device_fd, strerror(errno));
}
