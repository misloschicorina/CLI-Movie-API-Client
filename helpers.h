#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000

// Show a system error and exit
void error(const char *msg);

// Add a line to an HTTP message
void compute_message(char *message, const char *line);

// Open a TCP connection to the server
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// Close an open connection
void close_connection(int sockfd);

// Send a full message to the server
void send_to_server(int sockfd, char *message);

// Receive the full response from the server
char *receive_from_server(int sockfd);

// Extract the JSON object from a response string
char *basic_extract_json_response(char *str);

#endif
