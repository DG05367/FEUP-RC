#ifndef PHYSICAL_LAYER_H
#define PHYSICAL_LAYER_H

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */

void open_serial_port(char* port, int* fd);

int close_serial_port(int fd);

#endif