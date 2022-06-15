#ifndef PARSER_h
#define PARSER_h

#define STRING_LENGTH 256

typedef struct {
    char* user;
    char* password;
    char* hostname;
    char* file_path;
    char* ip;
    int data_port;
    int data_socket;
    long size;
    
} info;

/**
 * Parse user input and create a new info structure with the information gotten.
*/
info* arguments(char* input);

int verify_input(const char *input);

/**
* Parse received message as reply for pasv command and get the port to the data socket.
*/
int pasv_to_port(char* message);

/**
 * Parse reply size message to get the file size.
*/
int size(char* reply);

#endif