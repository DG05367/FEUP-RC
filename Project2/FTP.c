#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include "parser.h"
#include "FTP.h"

extern info *connection;

int send_message(int sockfd, char *message, char *parameters) {
    int bytes;
    char *full_message = malloc(STRING_LENGTH * sizeof(char));

    memset(full_message, 0, STRING_LENGTH);
    strcat(full_message, message);

    if (parameters != NULL)
        strcat(full_message, parameters);

    strcat(full_message, "\r\n");
    bytes = write(sockfd, full_message, strlen(full_message)); // Send a string to the server.

    return bytes;
}

int read_reply(int sockfd, char *code) {
    int bytes = 0, i = 0, state = 0, finish = 0;
    char maybecode[3], buf;

    memset(code, 0, 3);

    do {
        bytes += read(sockfd, &buf, 1);
        printf("%c", buf);

        switch (state) {
        case 0:
            code[i] = buf;
            i++;

            if (i > 3) {
                if (buf != ' ') {
                    state = 1;
                    i = 0;
                }

                else
                    state = 2;
            }

            break;

        case 1:
            if (isdigit(buf)) {
                maybecode[i] = buf;
                i++;

                if (i == 3) {
                    state = 3;
                    i = 0;
                }
            }

            break;

        case 2:
            if (buf == '\n')
                finish = 1;

            break;

        case 3:
            if ((maybecode[0] == code[0]) && (maybecode[1] == code[1]) && (maybecode[2] == code[2])) {
                if (buf == '-')
                    state = 1;

                else
                    state = 2;
            }

            else
                state = 1;

            break;
        };
    } while (finish != 1);

    return bytes;
}

int read_data(int sockfd, char *reply) {
    int bytes = 0;

    memset(reply, 0, STRING_LENGTH);
    bytes = read(sockfd, reply, STRING_LENGTH);

    return bytes;
}

int get_reply_code(int sockfd, char *reply) {
    int code_reply;

    code_reply = (int)reply[0] - '0';
    
    if(code_reply == 5) {
        close(sockfd);
        exit(1);
    }

    return code_reply;
}

int read_other_reply(int sockfd, char *reply, char *message) {
    int bytes;
    char *all_reply = malloc(STRING_LENGTH * sizeof(char));
    
    memset(all_reply, 0, STRING_LENGTH);
    bytes = read(sockfd, all_reply, STRING_LENGTH);
    printf("%s", all_reply);
    
    if(bytes > 0) {
        memset(reply, 0, 3);
        reply[0] = all_reply[0];
        reply[1] = all_reply[1];
        reply[2] = all_reply[2];

        if(strcmp(message, "pasv") == 0)
            pasv_to_port(all_reply);
        
        else
            size(all_reply);
    }

    return bytes;
}

int communication(int sockfd, char *message, char *parameters) {
    char *reply = malloc(3 * sizeof(char));
    int final_code, bytes;

    do {
        send_message(sockfd, message, parameters);

        do {
            if((strcmp(message, "pasv") == 0) || (strcmp(message, "SIZE ") == 0))
                bytes = read_other_reply(sockfd, reply, message);

            else
                bytes = read_reply(sockfd, reply);

            if(bytes > 0) {
                final_code = get_reply_code(sockfd, reply);

                if(final_code == 1 && strcmp("retr ", message) == 0) {
                    //get data and create file
                    get_file();
                    close(connection -> data_socket);
                }
            }

        } while(final_code == 1);

    } while(final_code == 4);

    return final_code;
}

int login_server(int sockfd) {
    printf(" > Username will be sent\n");
    int response = communication(sockfd, "user ", connection -> user);
    
    if(response != 3)
        return 1;

    printf(" > Username correct. Password will be sent\n");
    response = communication(sockfd, "pass ", connection -> password);
    
    if(response != 2) {
        fprintf(stderr, "%s", "User or password incorrect\n");
        return 1;
    }

    printf(" > User logged in\n");
    
    return 0;
}

char* get_ip_address() {
    struct hostent *h;
    char *ip = malloc(IP_LENGTH * sizeof(char));

    memset(ip, 0, IP_LENGTH);
    printf("[HOST: %s]\n", connection -> hostname);
    
    if((h = gethostbyname(connection -> hostname)) == NULL) {
        herror("gethostbyname");
        exit(1);
    }

    ip = inet_ntoa(*((struct in_addr*) h -> h_addr));
    
    return ip;
}

int open_connection(int port, int is_connection) {
    int sockfd, code;
    char *open_reply = malloc(3 * sizeof(char));
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(connection -> ip);
    server_addr.sin_port = htons(port);

    /*open an TCP socket*/
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error open socket connection ");
        exit(0);
    }

    /*connect to the server*/
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(0);
    }

    if(is_connection) {
        do {
            read_reply(sockfd, open_reply);
            code = get_reply_code(sockfd, open_reply);
        } while (code != 2);
    }

    return sockfd;
}

char* get_filename() {
    char *filename = malloc(STRING_LENGTH * sizeof(char));
    unsigned int i = 0, j = 0, state = 0;
    int length = strlen(connection -> file_path);

    memset(filename, 0, STRING_LENGTH);

    while(i < length) {
        switch(state) {
        case 0:
            if (connection -> file_path[i] != '/') {
                filename[j] = connection -> file_path[i];
                j++;
            }

            else
                state = 1;

            i++;

            break;
        
        case 1:
            memset(filename, 0, STRING_LENGTH);
            state = 0;
            j = 0;
            
            break;
        }
    }

    printf("[Filename %s]\n", filename);
    
    return filename;
}

int get_file() {
    char *filename = get_filename(), *message = malloc(STRING_LENGTH * sizeof(char));
    unsigned int bytes_read, total_bytes = 0;

    FILE *fd = fopen(filename, "w");

    if(fd == NULL) {
        fprintf(stderr, "%s", " > Error opening file to write!\n");
        exit(1);
    }

    printf(" > Reading file\n");
    
    while((bytes_read = read_data(connection->data_socket, message)) > 0) {
        total_bytes += bytes_read;
        fseek(fd, 0, SEEK_END);
        fwrite(message, sizeof(unsigned char), bytes_read, fd);
    }

    fclose(fd);
    
    if(total_bytes <= 0)
        fprintf(stderr, "%s", " > Error reading the file\n");
    
    return total_bytes;
}

int verify_file_size() {
    char *filename = get_filename();
    
    FILE *fd = fopen(filename, "r");
    fseek(fd, 0L, SEEK_END);
    
    int size = ftell(fd);
    
    fseek(fd, 0L, SEEK_SET);
    fclose(fd);
    
    if(size == connection -> size)
        return 1;
    
    else
        return 0;
}
