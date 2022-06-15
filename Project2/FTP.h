#ifndef FTP_h
#define FTP_h

#define SERVER_PORT 21
#define IP_LENGTH 16

/**
 * Send a message to the file descriptor sokfd 
 * with the following structure: message+param+"\r\n". 
 * The 2 final characters represent the end of message
*/
int send_message(int sockfd, char* message, char* param);

int read_reply(int sockfd, char* code);

int read_data(int sockfd, char *reply);

/**
 * Get first number from the response gotten
*/
int get_reply_code(int sockfd, char *reply);

int read_other_reply(int sockfd, char *reply, char *message);

/**
 * Send a message to the server and wait a response from it
*/
int communication(int sockfd, char *message, char *parameters);

/*
 * Send log in information to the server
*/
int login_server(int sockfd);

/**
 * get the ip address from the hostname given
*/
char* get_ip_address();

/**
 * open a new connection to the specified port
*/
int open_connection(int port, int is_connection);

/**
 * get the filename from the given path
*/
char* get_filename();

/**
 * create a local file and
 * get file data from the data socket
*/
int get_file();

int verify_file_size();

#endif