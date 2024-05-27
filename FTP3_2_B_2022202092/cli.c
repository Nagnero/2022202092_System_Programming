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

#define MAX_BUF 1024 * 1024  // Maximum buffer size for data

char* convert_addr_to_str(unsigned long ip_addr, unsigned int port);
void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag);
int conv_cmd(char *buff, char *cmd_buff);

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
int main(int argc, char **argv) {
    if (argc != 3)
        exit(EXIT_FAILURE);  // Ensure proper usage

    char *hostport;
    char buff[MAX_BUF], cmd_buff[MAX_BUF];
    struct sockaddr_in temp; // data connection
    struct sockaddr_in servaddr;  // Server address structure
    int sockfd_control, sockfd_data;  // Socket file descriptor

    /////////////////////// address and PORT for control connect ////////////////////////////
    memset(&servaddr, 0, sizeof(servaddr));  // Clear structure
    servaddr.sin_family = AF_INET;  // Set family to IPv4
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);  // Set IP address
    servaddr.sin_port = htons((uint16_t)atoi(argv[2]));  // Set port number
    /////////////////////////////////////////////////////////////////////////////////////////

    // make control connection
    sockfd_control = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket for IPv4 and TCP
    if (sockfd_control < 0) {
        perror("socket creation failed");  // Print error if socket creation fails
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd_control, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while(sockfd_control != -1) {
        memset(buff, 0, sizeof(buff)); // initialize buffer
        printf("> ");
        if (fgets(buff, sizeof(buff), stdin) == NULL) {
            printf("input error\n");
            break;
        }
        buff[strcspn(buff, "\n")] = '\0';

        /////////////////////// ip address and PORT for data connect ////////////////////////////
        memset(&temp, 0, sizeof(temp));  // Clear structure
        temp.sin_family = AF_INET;  // Set family to IPv4
        temp.sin_addr.s_addr = inet_addr(argv[1]);  // Set IP address
        temp.sin_port = htons((unsigned short)((rand()%20000) + 10001));  // Set port number
        /////////////////////////////////////////////////////////////////////////////////////////

        /* convert ls (including options) to NLST (including options) */
        if (conv_cmd(buff, cmd_buff) != 0) {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            write(sockfd_control, cmd_buff, strlen(cmd_buff));
            close(sockfd_control);
            exit(1);
        }

        // send PORT command with IP address and port changed port num
        if (strncmp(cmd_buff, "NLST", 4)) {
            char port_message[MAX_BUF];
            strcpy(port_message, "PORT ");
            strcat(port_message, hostport);
            if(write(sockfd_control, "PORT", strlen("PORT")) != 4) {
                write(STDERR_FILENO, "write() error!!\n", sizeof("write() error!!\n"));
                close(sockfd_control);
                exit(1);
            } 
        }

        if (strcmp(cmd_buff, "QUIT") == 0) {
            printf("221 Goodbye.\n");
            break;
        }

        hostport = convert_addr_to_str(temp.sin_addr.s_addr, temp.sin_port);
        printf("converting to %s\n", hostport);
        write(sockfd_control, hostport, strlen(hostport));
        
        //////////////////////////////// make data connection ////////////////////////////////////
        if ((sockfd_data = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket");
            return -1;
        }
        if (bind(sockfd_data, (struct sockaddr *)&temp, sizeof(temp)) < 0) {
            perror("bind");
            close(sockfd_data); 
            exit(1);
        }
        if (listen(sockfd_data, 5)) {
            perror("listen");
            exit(1);
        }
        
        // Accept the data connection from the server
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int clientfd = accept(sockfd_data, (struct sockaddr *)&client, &client_len);
        if (clientfd < 0) {
            perror("accept");
            close(sockfd_data);
            exit(1);
        }

        // Send the NLST command to the server
        if (write(sockfd_control, cmd_buff, strlen(cmd_buff)) != (ssize_t)strlen(cmd_buff)) {
            perror("write");
            close(sockfd_control);
            close(sockfd_data);
            close(clientfd);
            exit(1);
        }

        // Read data from the server and print it to stdout
        int n;
        while ((n = read(clientfd, buff, sizeof(buff) - 1)) > 0) {
            buff[n] = '\0';
            printf("%s", buff);
        }

        if (n < 0) {
            perror("read");
        }
        close(sockfd_data);
        //////////////////////////////////////////////////////////////////////////////////////////
    }
    close(sockfd_control);
    return 0;
}


 //자신의 IP주소와 임의의 포트번호를 PORT명령어에 붙는 형태로 변경
char* convert_addr_to_str(unsigned long ip_addr, unsigned int port) {
    char *addr = malloc(INET_ADDRSTRLEN + 6);

    if (addr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Extract each byte from the IP address
    unsigned char byte1 = (ip_addr >> 24) & 0xFF;
    unsigned char byte2 = (ip_addr >> 16) & 0xFF;
    unsigned char byte3 = (ip_addr >> 8) & 0xFF;
    unsigned char byte4 = ip_addr & 0xFF;

    // Extract high and low bytes from the port number
    unsigned char high_byte = (port >> 8) & 0xFF;
    unsigned char low_byte = port & 0xFF;

    // Convert the bytes and port to a formatted string
    sprintf(addr, "%u,%u,%u,%u,%u,%u", byte4, byte3, byte2, byte1, high_byte, low_byte);

    return addr;
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