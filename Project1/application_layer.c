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
#include "application_layer.h"

extern int is_start;

int send_message(unsigned char* message, int length) {
	int res;

	if(is_start == TRUE) {
		is_start = FALSE;
		res = llwrite(application.file_descriptor, message, &length);
	}

	else {
		unsigned char* data_package_ = data_package(message, &length);
		res = llwrite(application.file_descriptor, data_package_, &length);
	}

	if(res == FALSE)
		return FALSE;

	return TRUE;
}

unsigned char* get_message() {
	int length;
	unsigned char *message_read = llread(application.file_descriptor, &length), *only_data;
	static int file_received_size = 0;

	if(message_read == NULL || message_read[0] == DISC)
		return message_read;

	switch(message_read[0]) {
		case 0x02:
			file_parameters(message_read);
			break;

		case 0x01:
			only_data = get_only_data(message_read, &length);
			writefile(only_data, length);
			file_received_size += length;
			break;

		case 0x03:
			verify(message_read);
			break;
	}

	return message_read;
}

unsigned char* get_only_data(unsigned char* message_read, int* length) {
	unsigned int size = message_read[2] * 256 + message_read[3];
	unsigned char* only_data = malloc(size * sizeof(unsigned char));
	
    for(int j = 0; j < size; j++)
			only_data[j] = message_read[j + 4];
	
    *length = size;
	
    free(message_read);
	
    return only_data;
}

unsigned char* data_package(unsigned char* message, int* length) {
	unsigned char* data_package = malloc((*length + 4) * sizeof(unsigned char)), c = 0x01;
		static unsigned int n = 0;
		int l2 = *length / 256, l1 = *length % 256;

		data_package[0] = c;
		data_package[1] = (char) n;
		data_package[2] = l2;
		data_package[3] = l1;

		n++;
		n = (n % 256);
		
		for(int i = 0; i < *length; i++)
			data_package[i + 4] = message[i];

		*length = *length + 4;

		return data_package;
}

int START_END_package(unsigned char* package, int type) {
	int i = 0, j = 3;
	unsigned char file_size_char[4];
	unsigned int file_name_length = (unsigned int) strlen(file.file_name);

	//convert file size to a unsigned char array
	file_size_char[0] = (file.file_size >> 24) & 0xFF;
	file_size_char[1] = (file.file_size >> 16) & 0xFF;
	file_size_char[2] = (file.file_size >> 8) & 0xFF;
	file_size_char[3] = file.file_size & 0xFF;

	//size of file size unsigned char array, normally is 4
	int length_file_size = sizeof(file_size_char) / sizeof(file_size_char[0]);

	if(type == START_PACKAGE_TYPE)
		package[0] = 0x02;

	else if(type == END_PACKAGE_TYPE)
		package[0] = 0x03;

	else
		return -1;

	package[1] = 0x00;
	package[2] = length_file_size;

	//put file size unsigned char array in package array
	for(; i < length_file_size; i++,j++)
		package[j] = file_size_char[i];

	package[j] = 0x01;
	j++;
	package[j] =  file_name_length;
	j++;

	for(i = 0; i < file_name_length; i++,j++)
		package[j] = file.file_name[i];

	return j;
}

int file_size() {
	fseek(file.fp, 0L, SEEK_END);
	
    int file_size = (int) ftell(file.fp);
	
    if(file_size == -1)
		return -1;
	
    fseek(file.fp, 0L, SEEK_SET);
	
    return file_size;
}

void file_parameters(unsigned char* message) {
	int i = 0, file_name_size;
	unsigned char file_size[4];

	if(message[1] == 0x00) {
		int file_size_ = message[2];

		for(; i < file_size_; i++)
			file_size[i] = message[i + 3];

		file.file_size = (file_size[0] << 24) | (file_size[1] << 16) | (file_size[2] << 8) | (file_size[3]);
	}

	i += 3;
	
    if(message[i] == 0x01) {
		i++;
		file_name_size = message[i];
		i++;
		file.file_name = malloc ((file_name_size + 1) * sizeof(char));
		
        for(int j = 0; j < file_name_size; j++, i++)
			file.file_name[j] = message[i];

		file.file_name[file_name_size] = '\0';
	}

	file.fp = fopen((char*) file.file_name, "wb");
}

void readfile() {
	unsigned char* data = malloc(file.read_size * sizeof(unsigned char));
	int file_sent_size = 0;
	
	fseek(file.fp, 0, SEEK_SET);

	while(TRUE) {
		int res = 0;
		res = fread(data,sizeof(unsigned char), file.read_size, file.fp);
		
        if(res > 0) {
			if(send_message(data,res) == FALSE) {
				llclose(application.file_descriptor, -1);
				exit(-1);
			}

			file_sent_size += res;
		}

		if(feof(file.fp))
			break;
	}
}

void writefile(unsigned char* data, int read_size) {
	fseek(file.fp, 0, SEEK_END);
	fwrite(data, sizeof(unsigned char), read_size, file.fp);
}

int verify(unsigned char* message) {
	int file_size_size = message[2], file_size_total;
	unsigned char file_size_[4];

	for(int i = 0; i < file_size_size; i++)
		file_size_[i] = message[i + 3];

	file_size_total = (file_size_[0] << 24) | (file_size_[1] << 16) | (file_size_[2] << 8) | (file_size_[3]);

	if(file_size_total == file.file_size && file_size_total == file_size())
        return TRUE;

	else
		return FALSE;

	return FALSE;
}