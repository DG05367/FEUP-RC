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

int STOP = FALSE;
unsigned char flag_attempts = 1, flag_alarm = 1, flag_error = 0, duplicate = FALSE, control_values[] = {0x00, 0x40, 0x05, 0x85, 0x01, 0x81};

void alarm_handler() {
	flag_attempts++;

	printf("Alarm #%d\n", flag_attempts);

  	if(flag_attempts >= datalink.max_retransmissions)
	  flag_error = 1;

	flag_alarm = 1;
}

void state_machine(unsigned char c, int* state, unsigned char* frame, int *length, int frame_type) {
	switch(*state) {
		case S0:
			if(c == FLAG) {
				*state = S1;
				frame[*length - 1] = c;
			}
			break;
		
        case S1:
			if(c != FLAG) {
				frame[*length - 1] = c;
				
                if(*length == 4) {
					if((frame[1] ^ frame[2]) != frame[3])
						*state = SESC;

					else
						*state = S2;
				}
			}

			else {
				*length = 1;
				frame[*length - 1] = c;
			}
			break;
		
        case S2:
			frame[*length - 1] = c;
			
            if(c == FLAG) {
				STOP = TRUE;
				alarm(0);
				flag_alarm = 0;
			}
			
            else {
				if(frame_type == FRAME_S){
					*state = S0;
					*length = 0;
				}
			}
			break;

		case SESC:
			frame[*length - 1] = c;
			
            if(c == FLAG) {
				if(frame_type == FRAME_I){
					flag_error = 1;
					STOP = TRUE;
				}
				
                else{
					*state = S0;
					*length = 0;
				}
			}
	}
}

int SET_transmitter(int* fd) {
    unsigned char SET[5] = {FLAG, ADDRESS_T, CONTROL_T, BCC_T, FLAG}, elem, frame[5];
    int res, frame_length = 0, state = S0;
	
    (void) signal(SIGALRM, alarm_handler);
    
    while(flag_alarm == 1 && datalink.max_retransmissions > flag_attempts) {
        res = write(*fd, SET, 5);

        alarm(datalink.timeout);
        flag_alarm = 0;

        //Wait for UA signal.

        while(flag_alarm == 0 && STOP == FALSE) {
            res = read(*fd, &elem, 1);
       		
            if(res > 0) {
                frame_length++;
          		state_machine(elem, &state, frame, &frame_length, FRAME_S);
       		}
      }
  }

  if(flag_error == 1)
     return FALSE;

  else
    return TRUE;
}

int SET_receiver(int* fd) {
    unsigned char UA[5] = {FLAG, ADDRESS_T, CONTROL_R, BCC_R, FLAG}, elem, frame[5];
    int res, frame_length = 0, state = S0;

    while(STOP == FALSE) {
        res = read(*fd, &elem, 1);

        if(res > 0) {
		    frame_length++;
            state_machine(elem, &state, frame, &frame_length, FRAME_S);
      }
    }

	res = write(*fd, UA, 5);

	return TRUE;
}

int llopen(char* port, char* mode, int timeout, int max_retransmissions) {
  int fd, result;
  
  datalink.control_value = 0;
  datalink.timeout = timeout;
  datalink.max_retransmissions= max_retransmissions;
  
  open_serial_port(port, &fd);

  if(strcmp(mode, "w") == 0)
    result = SET_transmitter(&fd);

  else if(strcmp(mode, "r") == 0)
    result = SET_receiver(&fd);

  if(result == TRUE)
	return fd;
  
  else {
	llclose(fd, -1);
	return -1;
  }
}

int llwrite(int fd, unsigned char* message, int* length) {
	unsigned char* full_message = create_frame(message, length), elem, frame[5];
	int res, frame_length = 0, state = S0;

	if(*length < 0)
		return FALSE;

	flag_attempts = 1;
	flag_alarm = 1;
	flag_error = 0;
	STOP = FALSE;

	while(flag_alarm == 1 && datalink.max_retransmissions > flag_attempts) {
		res = write(fd, full_message, *length);
		alarm(datalink.timeout);
		flag_alarm = 0;
		
        //Wait for response signal.
		
        while(flag_alarm == 0 && STOP == FALSE) {
			res = read(fd, &elem, 1);
			
            if(res > 0) {
				frame_length++;
				state_machine(elem, &state, frame, &frame_length, FRAME_S);
			}
		}

		if(STOP == TRUE) {
			if(control_values[datalink.control_value + 4] == frame[2]) {
				flag_alarm = 1;
				flag_attempts = 1;
				flag_error = 0;
				STOP = FALSE;
				state = S0;
				frame_length = 0;
			}
		}
	}

	if (flag_error == 1)
		return FALSE;

	datalink.control_value = datalink.control_value ^ 1;
	
    return TRUE;
}

unsigned char* llread(int fd, int* length) {
    unsigned int message_array_length = 138;
	unsigned char elem, *message = malloc(message_array_length * sizeof(unsigned char)), *finish = malloc(1 * sizeof(unsigned char));
	int res, state = S0;

	*length = 0;
	flag_error = 0;
	STOP = FALSE;
	finish[0] = DISC;
	
    while(STOP == FALSE) {
		res = read(fd, &elem, 1);
		
        if(res > 0) {
			*length += 1;
			
            if(message_array_length <= *length) {
				message_array_length *= 2;
				message = realloc(message, message_array_length * sizeof(unsigned char));
			}

			state_machine(elem, &state, message, length, FRAME_I);
		}
	}

	if(message[4] == ADDRESS_R && flag_error != 1) {
		message = BCC1_random_error(message, *length);
		message = BCC2_random_error(message, *length);
	}

	if(flag_error == 1 || message[2] == CONTROL_T || message[2] == CONTROL_R)
		return NULL;

	if(message[2] == DISC) {
		llclose(fd, RECEIVER);
		return finish;
	}

	duplicate = (control_values[datalink.control_value] == message[2]) ? FALSE : TRUE;
	unsigned char temp = message[2], *no_head_message = remove_supervision_frame(message, length), *no_BCC2_message = BCC2(no_head_message, length);

	if(*length == -1) {
		if(duplicate == TRUE) {
			send_RR_REJ(fd, RR, temp);
			return NULL;
		}

		else {
			send_RR_REJ(fd, REJ, temp);
			return NULL;
		}
	}

	else {
		if(duplicate != TRUE) {
			datalink.control_value = send_RR_REJ(fd, RR, temp);
			return no_BCC2_message;
		}

		else {
			send_RR_REJ(fd, RR, temp);
			return NULL;
		}
	}
}

void llclose(int fd, int type) {
	unsigned char* received, UA[5] = {FLAG, ADDRESS_T, CONTROL_R, BCC_R, FLAG};

	if(type == TRANSMITTER) {
		received = send_DISC(fd);
        write(fd, UA, 5);
        sleep(1);
	}
	
	else if(type == RECEIVER)
		received = send_DISC(fd);

	close_serial_port(fd);
}

unsigned char* create_frame(unsigned char* message, int* length) {
	unsigned char BCC2 = 0x00, *new_message = malloc((*length + 1) * sizeof(unsigned char));
	int i = 0;
	
    for(; i < *length; i++) {
		new_message[i] = message[i];
		BCC2 ^= message[i];
	}
	
    new_message[*length] = BCC2;
	*length += 1;
	i = 0;

	unsigned char* stuffed_message = stuffing(new_message, length), *control_message = frame_header(stuffed_message, length);

	return control_message;
}

unsigned char* frame_header(unsigned char* stuffed_frame, int* length) {
	unsigned char* full_frame = malloc((*length + 5) * sizeof(unsigned char));

	full_frame[0] = FLAG;
	full_frame[1] = ADDRESS_T;
	full_frame[2] = control_values[datalink.control_value];
	full_frame[3] = full_frame[1] ^ full_frame[2];
	
    for(int i = 0; i < *length; i++)
		full_frame[i + 4] = stuffed_frame[i];
	
    full_frame[*length + 4] = FLAG;
    *length +=  5;

	free(stuffed_frame);

	return full_frame;
}

unsigned char* remove_supervision_frame(unsigned char* message, int* length) {
	unsigned char* control_message = malloc((*length - 5) * sizeof(unsigned char));
	int i = 4, j = 0;

	for(; i < *length - 1; i++, j++)
		control_message[j] = message[i];

	*length -= 5;
	
    free(message);

	return control_message;
}

unsigned char* stuffing(unsigned char* message, int* length) {
	unsigned int array_length = *length;
    unsigned char* str = malloc(array_length * sizeof(unsigned char));
	int j = 0;

    for(int i = 0; i < *length; i++, j++) {
		if(j >= array_length) {
			array_length *= 2;
			str = realloc(str, array_length * sizeof(unsigned char));
		}

		if(message[i] == 0x7d) {
			str[j] = 0x7d;
			str[j + 1]= 0x5d;
			j++;
		}

		else if(message[i] ==  0x7e) {
			str[j] = 0x7d;
			str[j + 1] = 0x5e;
			j++;
		}

		else
			str[j] = message[i];
	}

	*length = j;
	
    free(message);

	return str;
}

unsigned char* destuffing(unsigned char* message, int* length) {
	unsigned int array_length = 133;
	unsigned char* str = malloc(array_length * sizeof(unsigned char));
	int new_length = 0;

	for(int i = 0; i < *length; i++) {
		new_length++;

		if(new_length >= array_length) {
			array_length *= 2;
			str = realloc(str, array_length * sizeof(unsigned char));
		}

		if(message[i] == 0x7d) {
			if(message[i + 1] == 0x5d) {
				str[new_length -1] = 0x7d;
				i++;
			}

			else if(message[i + 1] == 0x5e) {
				str[new_length-1] = 0x7e;
				i++;
			}
		}

		else
			str[new_length - 1] = message[i];
	}

	*length = new_length;
	
    free(message);
	
    return str;
}

unsigned char* BCC2(unsigned char* control_message, int* length) {
	unsigned char control_BCC2 = 0x00, *destuffed_message = destuffing(control_message, length);
	int i = 0;

	for(; i < *length - 1; i++)
		control_BCC2 ^= destuffed_message[i];

	if(destuffed_message[*length - 1] != control_BCC2) {
		*length = -1;
		return NULL;
	}

	*length -= 1;
	unsigned char* data_message = malloc(*length * sizeof(unsigned char));
	
    for(i = 0; i < *length; i++)
		data_message[i] = destuffed_message[i];

	free(destuffed_message);
	
    return data_message;
}

unsigned char* BCC1_random_error(unsigned char* package, int size_package) {
	unsigned char* messed_up_message = malloc(size_package * sizeof(unsigned char)), letter;
	int random = (rand() % 100) + 1;

	memcpy(messed_up_message, package, size_package);

	if(ERROR_PERCENTAGE_BCC1 >= random) {
		do {
			letter = (unsigned char) ('A' + (rand() % 256));
		} while(letter == messed_up_message[3]);

		messed_up_message[3] = letter;
		flag_error = 1;

		printf("BCC1 messed UP\n");
	}

	free(package);
	
	return messed_up_message;
}


unsigned char* BCC2_random_error(unsigned char* package, int size_package) {
	unsigned char* messed_up_message = malloc(size_package * sizeof(unsigned char)), letter;
	int random = (rand() % 100) + 1;

	memcpy(messed_up_message, package, size_package);

	if(ERROR_PERCENTAGE_BCC2 >= random) {
		//change data to have error in BCC2
		int i = (rand() % (size_package - 5)) + 4;
		
		do {
			letter = (unsigned char) ('A' + (rand() % 256));
		} while(letter == messed_up_message[i]);

		messed_up_message[i] = letter;

		printf("Data messed up\n");
	}

	free(package);
	
	return messed_up_message;
}

int send_RR_REJ(int fd, unsigned int type, unsigned char c) {
	unsigned char bool_val, response[5];

	response[0] = FLAG;
	response[1] = ADDRESS_T;
	response[4] = FLAG;
	
    if(c == 0x00)
		bool_val = 0;
	
    else
		bool_val = 1;

	switch (type) {
		case RR:
			response[2] = control_values[(bool_val ^ 1) + 2];
			break;

		case REJ:
			response[2] = control_values[bool_val + 4];
			break;
	}

	response[3] = response[1] ^ response[2];

 	write(fd, response, 5);

	return bool_val ^ 1;
}

unsigned char* send_DISC(int fd) {
	unsigned char elem, *frame = malloc(5 * sizeof(unsigned char)), disc[5] = {FLAG, ADDRESS_T, DISC, ADDRESS_T ^ DISC, FLAG};
	int res, frame_length = 0, state = 0;
	 
    flag_attempts =1;
	flag_alarm = 1;
	flag_error = 0;
	STOP = FALSE;

	while(flag_alarm == 1 && datalink.max_retransmissions > flag_attempts) {
        res = write(fd, disc, 5);
        alarm(datalink.timeout);
		flag_alarm = 0;

		while(flag_alarm == 0 && STOP == FALSE) {
            res = read(fd,&elem,1);
			
            if(res > 0) {
                frame_length++;
				state_machine(elem, &state, frame, &frame_length, FRAME_S);
			}
		}
	}

	return frame;
}