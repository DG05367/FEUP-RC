#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physical_layer.h"
#include "datalink_layer.h"
#include "application_layer.h"

int is_start = FALSE;

int main(int argc, char** argv) {
	int start_end_max_size;
	unsigned char *start_package, *end_package, *message, null_val[] = {0xAA};

	if((argc < 3) || ((strcmp("/dev/ttyS0", argv[1]) !=0 ) && (strcmp("/dev/ttyS1", argv[1]) != 0))) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS0\n");
      exit(1);
    }

    if(strcmp("w", argv[2]) != 0 && strcmp("r", argv[2]) != 0) {
        printf("Usage:\tinvalid read/write mode. a correct mode (w/r).\n\tex: nserial /dev/ttyS0 w\n");
        exit(1);
    }

	srand(time(NULL));
	application.status = argv[2];

	if(strcmp("w", application.status) == 0) {
		if(argv[3] == NULL || argv[4] == NULL) {
			printf("You need to specify the file and the size to read\n");
			exit(1);
		}

		if(argv[5] == NULL || argv[6] == NULL) {
			printf("You need to specify the timeout the maximum of retransmissions\n");
			exit(1);
		}

		application.file_descriptor = llopen(argv[1], application.status, atoi(argv[5]), atoi(argv[6]));

		if(application.file_descriptor > 0) {
			file.file_name = (char*) argv[3];
			file.read_size = atoi(argv[4]);

			file.fp = fopen((char*) file.file_name, "rb");

			if(file.fp == NULL) {
					printf("Invalid file!\n");
					exit(-1);
			}
			
            if((file.file_size = file_size()) == -1)
				return FALSE;
			
            start_end_max_size = 2 * (strlen(file.file_name) + 9 ) + MAX_SIZE_LINK; //max size for start/end package;
			start_package = malloc(start_end_max_size * sizeof(unsigned char));

			int start_created_size = START_END_package(start_package, START_PACKAGE_TYPE);

			if(start_created_size == -1) {
                printf("Error creating start packet\n");
				exit(-1);
			}

			is_start = TRUE;
			
            if(send_message(start_package, start_created_size) == FALSE)
				llclose(application.file_descriptor, -1);

			readfile();

			is_start = TRUE;
			end_package = malloc(start_end_max_size * sizeof(unsigned char));

			int end_package_size = START_END_package(end_package, END_PACKAGE_TYPE);
            
			is_start = TRUE;
				
            if(send_message(end_package, end_package_size) == FALSE)
				llclose(application.file_descriptor, -1);

			llclose(application.file_descriptor, TRANSMITTER);
			}
		}

		else if(strcmp("r", application.status) == 0) {
			if(argv[3] == NULL || argv[4] == NULL) {
					printf("You need to specify the timeout the maximum of retransmissions\n");
					exit(1);
			}

			application.file_descriptor = llopen(argv[1], application.status, atoi(argv[3]), atoi(argv[4]));
			
            if(application.file_descriptor > 0) {
				do {
					message = get_message();

					if(message == NULL)
						message = null_val;

				} while(message[0] != DISC);
			}
		}
        
        else {
			printf("Error opening serial port!\n");
			return 1;
		}
	
	return 0;
}