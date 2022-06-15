#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "parser.h"

static info connection;

info* arguments(char* input) {
    if(!verify_input(input))
        return NULL;

    char element;
    unsigned int i = 6, word_index = 0, state = 0, input_length = strlen(input);
    
    connection.user = malloc(STRING_LENGTH * sizeof(char));
    memset(connection.user, 0, STRING_LENGTH);

    connection.password = malloc(STRING_LENGTH * sizeof(char));
    memset(connection.password, 0, STRING_LENGTH);
    
    connection.hostname = malloc(STRING_LENGTH * sizeof(char));
    memset(connection.hostname, 0, STRING_LENGTH);
    
    connection.file_path = malloc(STRING_LENGTH * sizeof(char));
    memset(connection.file_path, 0, STRING_LENGTH);

    while(i < input_length) {
        element = input[i];

        switch(state) {
            case 0:
                if(element == ':') {
                    word_index = 0;
                    state = 1;
                    
                } 
                
                else {
                    connection.user[word_index] = element;
                    word_index++;
                }

                break;
            
            case 1:
                if(element == '@') {
                    word_index = 0;
                    state = 2;
                    
                } 
                
                else {
                    connection.password[word_index] = element;
                    word_index++;
                }
                
                break;
            
            case 2:
                if(element == '/'){
                    word_index = 0;
                    state = 3;
                } 
                
                else {
                    connection.hostname[word_index] = element;
                    word_index++;
                }
                
                break;
            
            case 3:
                connection.file_path[word_index] = element;
                word_index++;
                
                break;
        }
        i++;
    }

    printf("[Username: %s]\n", connection.user);
    printf("[Password: %s]\n", connection.password);

    return &connection;
}

int verify_input(const char *input) {
    int status;
    regex_t reg;
    char* RE = "ftp://.*:.*@.*/.*";
    
    if(regcomp(&reg, RE, REG_EXTENDED|REG_NOSUB) != 0)
        return(0); // Report error

    status = regexec(&reg, input, (size_t) 0, NULL, 0);
    regfree(&reg);

    if(status != 0)
        return(0); // Report error

    return(1);
}


int pasv_to_port(char* message) {
    char pasv_codes[24];
    unsigned int length = strlen(message) - 29;
    int host, port_1, port_2;

    for(int i = 0; i < length; i++)
        pasv_codes[i] = message[i + 26];
   
    sscanf(pasv_codes, "(%d, %d, %d, %d, %d, %d)", &host, &host, &host, &host, &port_1, &port_2);
    connection.data_port = (port_1 * 256 + port_2);

    return 0;
}

int size(char* reply) {
    int host;
    
    sscanf(reply, "%d %ld", &host, &connection.size);
    printf(" > File size: %ld\n", connection.size);
    
    return 0;
}
