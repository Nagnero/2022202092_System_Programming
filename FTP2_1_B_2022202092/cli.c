//////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                //
// Date : 2024/05/01                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #2-1 ( ftp server )        //
// Description : The program work as Linux terminal commands ls,    /
//              quit command. This file work for getting user input //
//              and transfer it to FTP commands and send to server  //
//              to execute appropriate work                         //
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/getopt_core.h>
#include <arpa/inet.h>

#define MAX_BUFF 1000000
#define RCV_BUFF 1000000

int socketConnection(char* server_addr, int port);
void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag);
int conv_cmd(char *buff, char *cmd_buff);
void process_result(char *result);

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: argc -> Number of command-line arguments                  //
//        argv -> Array of command-line arguments                   //
//                                                                  //
// Output: int - Status code (0 for success, non-zero for failure)  //
//                                                                  //
// Purpose: Initializes a network connection, processes user input, //
//          sends commands to the server, and handles server        //
//          responses. This loop continues until an error occurs or //
//          the "QUIT" command is received from the server.         //
//////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
    char buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int n;
    int sockfd = socketConnection(argv[1], atoi(argv[2])); // get connection and save socket number

    while(sockfd != -1) {
        memset(buff, 0, sizeof(buff)); // initialize buffer
        printf("> ");
        if (fgets(buff, sizeof(buff), stdin) == NULL) {
            printf("input error\n");
            break;
        }
        buff[strcspn(buff, "\n")] = '\0';
        
        /* convert ls (including options) to NLST (including options) */
        if (conv_cmd(buff, cmd_buff) != 0) {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            write(sockfd, cmd_buff, strlen(cmd_buff));
            close(sockfd);
            exit(1);
        }

        n = strlen(cmd_buff);
        if(write(sockfd, cmd_buff, n) != n) {
            write(STDERR_FILENO, "write() error!!\n", sizeof("write() error!!\n"));
            close(sockfd);
            exit(1);
        } 
        if((n = read(sockfd,rcv_buff, RCV_BUFF-1)) < 0) {
            write(STDERR_FILENO,"read() error\n", sizeof("read() error\n"));
            close(sockfd);
            exit(1);
        }
        rcv_buff[n] = '\0'; // Ensure null-terminated string

        if (strncmp(rcv_buff, "QUIT", 4) == 0) {
            write(STDOUT_FILENO, "Program quit!!\n", strlen("Program quit!!\n"));
            close(sockfd);
            exit(1);
        }

        if (strncmp(rcv_buff, "cmd_process() err!!\n", sizeof("cmd_process() err!!\n")) == 0) {
            write(STDOUT_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: unknown command\n\n", sizeof("Error: unknown command\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: arguments is not required\n\n", sizeof("Error: arguments is not required\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: invalid option\n\n", sizeof("Error: invalid option\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: too many argument\n\n", sizeof("Error: too many argument\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: no such file exist\n\n", sizeof("Error: no such file exist\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        else if (strncmp(rcv_buff, "Error: Permission denied\n\n", sizeof("Error: Permission denied\n\n")) == 0) {
            write(STDERR_FILENO, rcv_buff, strlen(rcv_buff));
            close(sockfd);
            exit(1);
        }
        

        /*display ls(including options) command result */
        process_result(rcv_buff);
        //memset(rcv_buff, 0, RCV_BUFF);
        memset(cmd_buff, 0, MAX_BUFF);
        //memset(buff, 0, MAX_BUFF);
    }
    
    close(sockfd);
    return 0;
}

//////////////////////////////////////////////////////////////////////
// socketConnection                                                 //
// =================================================================//
// Input: server_addr -> pointer to a string representing the       //
//        server's IP address                                       //
//        port -> integer representing the server's port number     //
//                                                                  //
// Output: int - A file descriptor for the socket on success;       //
//               -1 on failure                                      //
//                                                                  //
// Purpose: Establish connection to a specified server address      //
//          and port. This function creates a socket, sets up the   //
//          server address structure, and attempts a connection.    //
//          If any step fails, it returns -1 and closes the socket  //
//          if it was opened.                                       //
//////////////////////////////////////////////////////////////////////
int socketConnection(char* server_addr, int port) {
    int sockfd;
    struct sockaddr_in server;

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    bzero((char*)&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_addr);
    server.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


//////////////////////////////////////////////////////////////////////
// parse_options                                                    //
// =================================================================//
// Input: argc -> Number of command-line arguments                  //
//        argv -> Array of command-line arguments                   //
//        aflag -> Pointer to the flag for 'a' option               //
//        lflag -> Pointer to the flag for 'l' option               //
//        oflag -> Pointer to the flag for other unrecognized options //
//                                                                  //
// Output: None (void function)                                     //
//                                                                  //
// Purpose: Parses command-line options and sets flags for handling //
//          specific command options like 'a' and 'l'. Unrecognized //
//          options set an error flag.                              //
//////////////////////////////////////////////////////////////////////
void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') { // check if it is option
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        *aflag = 1;
                        break;
                    case 'l':
                        *lflag = 1;
                        break;
                    default:
                        *oflag = 1; 
                        return; 
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// conv_cmd                                                         //
// =================================================================//
// Input: buff -> pointer to the input buffer containing the user's //
//                command line input                                //
//        cmd_buff -> pointer to the buffer where the converted     //
//                    command will be stored                        //
//                                                                  //
// Output: int - Returns 0 if successful, error flags are handled   //
//               internally and affect command behavior             //
//                                                                  //
// Purpose: Converts user commands into server-compatible commands.//
//          This includes converting 'ls' commands to 'NLST' and    //
//          handling various flags such as '-a' and '-l'. It parses //
//          the command and options, and builds a new command       //
//          string based on the input. Errors in options or command //
//          structure set specific flags that modify the final      //
//          command or trigger error messages.                      //
/////////////////////////////////////////////////////////////////////
int conv_cmd(char *buff, char *cmd_buff) {
    int aflag = 0, lflag = 0;            // flag for option management
    int oflag = 0, xflag = 0, qflag = 0; // flag for error management

    char *token;
    int argc = 0;
    char *argv[100];
    // parse data from client buffer
    token = strtok(buff, " \n");
    while (token != NULL) {
        argv[argc++] = token;      // Assign the token to argv and increment argc
        token = strtok(NULL, " \n"); // Continue tokenizing the string
    }
    argv[argc] = NULL;
    for (int i = 0; i > argc; i++)
        strcat("%s\n", argv[i]);

    // If first argument is "ls"
    if (strcmp(argv[0], "ls") == 0) {
        parse_options(argc, argv, &aflag, &lflag, &oflag);

        strcpy(cmd_buff, "NLST"); // Start building the NLST command

        // Append appropriate flags to the command
        if (aflag == 1 && lflag == 0) 
            strcat(cmd_buff, " -a");        
        else if (aflag == 0 && lflag == 1) 
            strcat(cmd_buff, " -l");
        else if (aflag == 1 && lflag == 1) 
            strcat(cmd_buff, " -al");
        
        // Append path if specified
        char* temp = argv[argc - 1];
        if (strcmp(temp, "ls") != 0 && temp[0] != '-') {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, temp); // store path to temp
            if (argc > 2)
                if (strcmp(argv[argc - 2], "ls") != 0 && argv[argc - 2][0] != '-')
                    xflag = 1; // set too many argument error flag
        }
        else if (strcmp(temp, "ls") == 0 && argc > 1) {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, temp);
        }
            
        
    }
    // 'quit' command
    else if (strcmp(argv[0], "quit") == 0) {
        // 'quit' command does not take any options or path
        strcpy(cmd_buff, "QUIT");
        if (argc != 1)
            qflag = 1;
    }
    // Unknown command
    else {
        strcpy(cmd_buff, "Error: !");
    }

    // process error flag
    if (qflag)
        oflag ? strcpy(cmd_buff, "Error: o") : strcpy(cmd_buff, "Error: q");
    else if (oflag)
        strcpy(cmd_buff, "Error: o");
    else if (xflag)
        strcpy(cmd_buff, "Error: x");

    return 0;
}

//////////////////////////////////////////////////////////////////////
// process_result                                                   //
// =================================================================//
// Input: result -> pointer to the result string to be displayed    //
//                                                                  //
// Output: None (void function)                                     //
//                                                                  //
// Purpose: Display the result string and clear its content.        //
//////////////////////////////////////////////////////////////////////
void process_result(char *result) {
    printf("%s\n", result);
    memset(result, 0, sizeof(*result));
}

