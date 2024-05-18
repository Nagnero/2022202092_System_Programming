//////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                //
// Date : 2024/05/18                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #3-1 ( ftp server )        //
// Description : This file is part of a client-server application   //
//               that connects to a server, sends and receives      //
//               data, and handles user authentication.             //
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUF 20  // Maximum buffer size for data

void log_in(int sockfd);  // Function prototype for log_in

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: argc -> Number of command line arguments                  //
//        argv -> Array of command line argument strings            //
//                                                                  //
// Output: int - Returns 0 for normal termination                   //
//               Returns 1 for error termination                    //
//                                                                  //
// Purpose: Connect to a server, send commands received from the    //
//          user, and handle server responses. Manages signal       //
//          handlers for process control and graceful termination.  //
//////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    if (argc != 3)
        exit(EXIT_FAILURE);  // Ensure proper usage

    int sockfd;  // Socket file descriptor
    struct sockaddr_in servaddr;  // Server address structure

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket for IPv4 and TCP
    if (sockfd < 0) {
        perror("socket creation failed");  // Print error if socket creation fails
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));  // Clear structure
    servaddr.sin_family = AF_INET;  // Set family to IPv4
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);  // Set IP address
    servaddr.sin_port = htons(atoi(argv[2]));  // Set port number

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Send IP and port to server
    char ip_port_str[2 * MAX_BUF + 1];  // Buffer to hold IP and port
    snprintf(ip_port_str, sizeof(ip_port_str), "%s\n%d", argv[1], htons(atoi(argv[2])));
    write(sockfd, ip_port_str, strlen(ip_port_str));
    
    // Receive connection result
    char buf[MAX_BUF];  // Buffer for server response
    int n = read(sockfd, buf, MAX_BUF);  // Read server response
    buf[n] = '\0';  // Null-terminate string
    if (strncmp(buf, "REJECTION", strlen("REJECTION")) == 0) {
        printf("** Connection refused **\n");
        close(sockfd);
        return 0;
    } else if (strncmp(buf, "ACCEPTED", strlen("ACCEPTED")) == 0) {
        printf("** It is connected to Server **\n");
    }

    log_in(sockfd);  // Proceed to log in
    close(sockfd);  // Close socket
    return 0;
}


void log_in(int sockfd) {
    int n;
    char user[MAX_BUF], passwd[MAX_BUF], buf[MAX_BUF];

    while(1) {
        char buffer[MAX_BUF * 2 + 1];  // Buffer to hold user and password
        printf("Input ID : ");  // Prompt for user ID
        fgets(user, MAX_BUF, stdin);  // Read user ID
        user[strcspn(user, "\n")] = 0;  // Remove newline character
        printf("Input Password : ");  // Prompt for password
        fgets(passwd, MAX_BUF, stdin);  // Read password
        passwd[strcspn(passwd, "\n")] = 0;  // Remove newline character
        snprintf(buffer, MAX_BUF * 2 + 1, "%s\n%s", user, passwd);  // Format user and password
        write(sockfd, buffer, MAX_BUF * 2 + 1);  // Send to server
        
        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';
        if(strcmp(buf, "OK") == 0) {
            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            if (strncmp(buf, "OK", strlen("OK")) == 0) {
                printf("** User '%s' logged in **\n", user);
                return;
            } else if (strncmp(buf, "FAIL", strlen("FAIL")) == 0) {
                printf("** Log-in failed **\n");
            } else if (strncmp(buf, "DISCONNECTION", strlen("DISCONNECTION")) == 0) {
                printf("** Connection closed **\n");
                return;
            }
        }
    }
}

