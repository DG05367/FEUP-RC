#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include "FTP.h"

info *connection;

int main(int argc, char **argv) {
    int command_socket, reply_command;

    if (argv[1] == NULL) {
        printf(" > Error. Use the structure: ftp://<username>:<password>@<host>/<file>\n");
        exit(1);
    }

    if ((connection = arguments(argv[1])) == NULL) {
        printf(" > Input values are not valid! Please try again\n");
        exit(1);
    }

    connection -> ip = get_ip_address();
    printf("[IP address: %s]\n", connection -> ip);

    // Open connection to the server to send commands.
    command_socket = open_connection(SERVER_PORT, 1);

    // Error logging in.
    if(login_server(command_socket) != 0) {
        fprintf(stderr, "%s", " > Error logging in. Please try again!\n");
        close(command_socket);
        exit(1);
    }

    // Send size command.
    printf(" > Size command will be sent\n");
    communication(command_socket, "SIZE ", connection -> file_path);

    // Send pasv and get port to receive the file.
    printf(" > Pasv command will be sent\n");
    communication(command_socket, "pasv", NULL);

    // Open the new socket to receive the file.
    printf(" > Data socket will be opened\n");
    connection -> data_socket = open_connection(connection -> data_port, 0);

    // Send retrieve command to receive the file.
    printf(" > Retr message will be sent\n");
    reply_command = communication(command_socket, "retr ", connection -> file_path);
    close(command_socket);
    
    if(reply_command != 2) {
        fprintf(stderr, "%s", " > Error getting file or sending retr\n");
        exit(1);
    }

    else {
        if(verify_file_size()) {
            exit(0);
        }

        else {
            fprintf(stderr, "%s\n", " > The received file is probably damaged\n");
            exit(1);
        }
    }
}