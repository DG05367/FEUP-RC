#ifndef DATALINK_LAYER_H
#define DATALINK_LAYER_H

#define FALSE 0
#define TRUE 1

#define RECEIVER 0
#define TRANSMITTER 1

#define FLAG 0x7E
#define ADDRESS_T 0x03
#define ADDRESS_R 0x01
#define CONTROL_T 0x03
#define CONTROL_R 0x07
#define BCC_T 0x00
#define BCC_R 0x04

#define CONTROL_START 2
#define CONTROL_END 3

#define FRAME_S 0
#define FRAME_I 1

#define REJ 0
#define RR 1

#define DISC 0x0B

#define ERROR_PERCENTAGE_BCC1 0
#define ERROR_PERCENTAGE_BCC2 0

#define S0 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define SESC 5

typedef struct {
	struct termios oldtio;
	struct termios newtio;
	unsigned char control_value;
	unsigned int timeout;
	unsigned int max_retransmissions;
} Datalink;

Datalink datalink;

void alarm_handler();

void state_machine(unsigned char c, int* state, unsigned char* frame, int* length, int frame_type);

int SET_transmitter(int* fd);

int SET_receiver(int* fd);

int llopen(char* port, char* mode, int timeout, int max_retransmissions);

int llwrite(int fd, unsigned char* message, int* length);

unsigned char* llread(int fd, int* length);

void llclose(int fd, int type);

unsigned char* create_frame(unsigned char* message, int* length);

unsigned char* frame_header(unsigned char* stuffed_frame, int* length);

unsigned char* remove_supervision_frame(unsigned char* message, int* length);

unsigned char* stuffing(unsigned char* message, int* length);

unsigned char* destuffing(unsigned char* message, int* length);

unsigned char* BCC2(unsigned char* control_message, int* length);

unsigned char* BCC1_random_error(unsigned char* package, int size_package);

unsigned char* BCC2_random_error(unsigned char* package, int size_package);

int send_RR_REJ(int fd, unsigned int type, unsigned char c);

unsigned char* send_DISC(int fd);

#endif