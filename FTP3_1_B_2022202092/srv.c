//////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                //
// Date : 2024/05/18                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #3-1 (ftp server)          //
// Description : This file is part of a server application that     //
//               listens for connections, authenticates clients,    //
//               and handles user commands.                         //
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>

#define MAX_BUF 20

int log_auth(int connfd);  // Function prototype for log_auth
int user_match(char *user, char *passwd);  // Function prototype for user_match

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: argc -> Number of command line arguments                  //
//        argv -> Array of command line argument strings            //
//                                                                  //
// Output: int - Returns 0 for normal termination                   //
//               Returns 1 for error termination                    //
//                                                                  //
// Purpose: Initializes the server to listen on a specified port,   //
//          accepts client connections, and handles client          //
//          authentication and command execution.                   //
//////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    FILE *fp_checkIP; // FILE stream to check client’s IP

    fp_checkIP = fopen("access.txt", "r");  // Open file to read client IP access list

    socklen_t clilen = sizeof(cliaddr);  // Length of client address
    listenfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket for IPv4 and TCP
    if (listenfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));  // Clear structure
    servaddr.sin_family = AF_INET;  // Set family to IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on any IP address
    servaddr.sin_port = htons(atoi(argv[1]));  // Set port number

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    listen(listenfd, 5);
    while(1) {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
            continue;
        }

        char ip_port_str[2 * MAX_BUF + 1];  // Buffer to hold IP and port
        char ip_str[MAX_BUF];  // Buffer to hold IP
        char port_str[MAX_BUF];  // Buffer to hold port
        printf("** Client is trying to connect **\n");
        read(connfd, ip_port_str, sizeof(ip_port_str));  // Read data from client
        sscanf(ip_port_str, "%s\n%s", ip_str, port_str);  // Extract IP and port from received data
        printf(" - IP:    %s\n", ip_str);
        printf(" - Port:  %s\n", port_str);

        char line[100];  // Buffer to hold line from file
        int access_granted = 0;  // Flag for access permission
        while(fgets(line, sizeof(line), fp_checkIP) != NULL) {  // Read lines from file
            if (!strcmp(line, "\n"))
                break;
            line[strcspn(line, "\n")] = 0;  // Remove newline character
            char *pattern = strtok(line, ".");  // Tokenize IP pattern
            char *ip_segment = strtok(ip_str, ".");  // Tokenize received IP
            int match = 1;  // Flag for IP match

            while (pattern != NULL && ip_segment != NULL) {  // Check each segment of IP
                if (strcmp(pattern, "*") != 0 && strcmp(pattern, ip_segment) != 0) {
                    match = 0;
                    break;
                }
                pattern = strtok(NULL, ".");
                ip_segment = strtok(NULL, ".");
            }

            if (match) {
                access_granted = 1;
                break;
            }
        }

        if (access_granted) {
            printf("** Client is connected **\n");
            write(connfd, "ACCEPTED", strlen("ACCEPTED"));
        } else {
            write(connfd, "REJECTION", strlen("REJECTION"));
            printf("** It is NOT authenticated client **\n");
            close(connfd);
            continue;
        }

        if (log_auth(connfd) == 0) { // if 3 times fail (ok : 1, fail : 0)
            printf("** Fail to log-in **\n");
            close(connfd);
            continue;
        }
        printf("** Success to log-in **\n");
        close(connfd);
    }

    fclose(fp_checkIP);
    close(listenfd);  // Close the listening socket
    return 0;
}

//////////////////////////////////////////////////////////////////////
// log_auth                                                         //
// =================================================================//
// Input: connfd -> Connection file descriptor                      //
//                                                                  //
// Output: int - Returns 1 on successful authentication             //
//               Returns 0 on authentication failure                //
//                                                                  //
// Purpose: Authenticates a user by verifying provided credentials  //
//          against stored user data. It allows multiple attempts   //
//          and disconnects the user after repeated failures.       //
//////////////////////////////////////////////////////////////////////
int log_auth(int connfd) {
    char user[MAX_BUF], passwd[MAX_BUF];  // Buffers for user and password
    int n, count = 1;

    while (1) {
        char buffer[MAX_BUF * 2 + 1];
        n = read(connfd, buffer, sizeof(buffer));
        buffer[n] = '\0'; // Null terminate the string

        char *newline_pos = strchr(buffer, '\n');
        if (newline_pos) {
            *newline_pos = '\0'; // Null terminate the user string
            strncpy(user, buffer, MAX_BUF);
            strncpy(passwd, newline_pos + 1, MAX_BUF);
        }

        write(connfd, "OK", MAX_BUF);
        printf("** User is trying to log-in (%d/3) **\n", count);
        if ((n = user_match(user, passwd)) == 1) {
            write(connfd, "OK", 3);
            break;
        }
        else if (n == 0) { // user math fail
            printf("** Log-in failed **\n");

            if (count >= 3) {
                write(connfd, "DISCONNECTION", MAX_BUF);
                return 0; // return 0 on fail
            }
            write(connfd, "FAIL", MAX_BUF);
            count++;
            continue;
        }
    }
    return 1; // return 1 on success
}

//////////////////////////////////////////////////////////////////////
// user_match                                                       //
// =================================================================//
// Input: user -> User name string                                  //
//        passwd -> Password string                                 //
//                                                                  //
// Output: int - Returns 1 if credentials match                     //
//               Returns 0 if credentials do not match              //
//                                                                  //
// Purpose: Checks if the provided user name and password match     //
//          any stored credentials.                                 //
//////////////////////////////////////////////////////////////////////
int user_match(char *user, char *passwd) {
    FILE *fp;
    struct passwd *pw;

    fp = fopen("passwd", "r");
    while((pw = fgetpwent(fp)) != NULL) {
        if ((strcmp(user, pw->pw_name) == 0) && (strcmp(passwd, pw->pw_passwd)) == 0)
            return 1; // return 1 on success
        else
            continue;
    }

    return 0; // return 0 on fail
}
