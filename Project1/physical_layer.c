#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physical_layer.h"
#include "datalink_layer.h"

/* SET Serial Port Initilizations */
void open_serial_port(char* port, int* fd) {
    /*
      Open serial port device for reading and writing and not as controlling tty
      because we don't want to get killed if linenoise sends CTRL-C.
    */

    *fd = open(port, O_RDWR | O_NOCTTY );

	if(*fd < 0) {
        perror(port); 
        exit(-1);
    }

    if(tcgetattr(*fd, &datalink.oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&datalink.newtio, sizeof(datalink.newtio));
    datalink.newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    datalink.newtio.c_iflag = IGNPAR;
    datalink.newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    datalink.newtio.c_lflag = 0;

    datalink.newtio.c_cc[VTIME] = 1;   /* inter-character timer unused */
    datalink.newtio.c_cc[VMIN] = 0;   /* blocking read until 1 char received */

    tcflush(*fd, TCIOFLUSH);

    if(tcsetattr(*fd,TCSANOW, &datalink.newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
}

int close_serial_port(int fd) {
	if(tcsetattr(fd,TCSANOW, &datalink.oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    
    return 0;
}