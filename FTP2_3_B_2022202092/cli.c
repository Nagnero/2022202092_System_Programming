#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <bits/getopt_core.h>

#define BUF_SIZE 100000

void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag);
int conv_cmd(char *buff, char *cmd_buff);

int main(int argc, char **argv) {
    char buff[BUF_SIZE], cmd_buff[BUF_SIZE], rcv_buff[BUF_SIZE];
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    while (1) {
        write(STDOUT_FILENO, ">", 2);
        read(STDIN_FILENO, buff, BUF_SIZE);

        if (conv_cmd(buff, cmd_buff) != 0) {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            write(sockfd, cmd_buff, strlen(cmd_buff));
            close(sockfd);
            exit(1);
        }
        
        int n = strlen(cmd_buff);
        if(write(sockfd, cmd_buff, n) != n) {
            write(STDERR_FILENO, "write() error!!\n", sizeof("write() error!!\n"));
            close(sockfd);
            exit(1);
        } 
        
        if((n = read(sockfd, rcv_buff, BUF_SIZE-1)) < 0) {
            write(STDERR_FILENO,"read() error\n", sizeof("read() error\n"));
            close(sockfd);
            exit(1);
        }
        memset(buff, 0, BUF_SIZE);
        memset(cmd_buff, 0, BUF_SIZE);


        if (strncmp(rcv_buff, "QUIT", 4) == 0) {
            write(STDOUT_FILENO, "Program quit!!\n", strlen("Program quit!!\n"));
            close(sockfd);
            break;
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
        printf("%s\n", rcv_buff);
        memset(rcv_buff, 0, sizeof(rcv_buff));
    }

    close(sockfd);
    return 0;
}

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
// Purpose: Converts user commands into server-compatible commands. //
//          This includes converting 'ls' commands to 'NLST' and    //
//          handling various flags such as '-a' and '-l'. It parses //
//          the command and options, and builds a new command       //
//          string based on the input. Errors in options or command //
//          structure set specific flags that modify the final      //
//          command or trigger error messages.                      //
//////////////////////////////////////////////////////////////////////
int conv_cmd(char *buff, char *cmd_buff) {
    int aflag = 0, lflag = 0;            // flag for option management
    int oflag = 0, xflag = 0, yflag = 0, qflag = 0, rflag = 0; // flag for error management
    
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

    // for (int i = 0; i > argc; i++)
    //     write(STDOUT_FILENO, argv[i], sizeof(argv[i]));


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
    // 'dir' command
    else if (strcmp(argv[0], "dir") == 0) {
        // option check for dir
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // 'dir' command does not take any options, only a path
        strcpy(cmd_buff, "LIST");

        if (strcmp(argv[argc -1], "dir") != 0) {
            if (strcmp(argv[argc - 2], "dir") == 0) {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[argc - 1]); // store path to buffer
            }
            else
                qflag = 1;
        }
    }
    // 'pwd' command
    else if (strcmp(argv[0], "pwd") == 0) {
        // option check for pwd
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;
        
        // 'pwd' command does not take any option and path
        strcpy(cmd_buff, "PWD");

        if (strcmp(argv[argc - 1], "pwd") != 0) {
            qflag = 1;
        }
    }
    // 'cd' command
    else if (strcmp(argv[0], "cd") == 0) {
        // option check for cd
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "CWD");
        if (strcmp(argv[argc - 1], "cd") != 0) {
            if (strcmp(argv[argc - 2], "cd") == 0) {
                if (strcmp(argv[argc - 1], "..") == 0)
                    strcpy(cmd_buff, "CDUP");
                else {
                    strcat(cmd_buff, " ");
                    strcat(cmd_buff, argv[argc - 1]); // store path to buffer
                }
            }
            else
                xflag = 1;
        }
        else
            yflag = 1;
    }
    // 'mkdir' command
    else if (strcmp(argv[0], "mkdir") == 0) {
        // option check for mkdir
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "MKD");

        if (strcmp(argv[argc - 1], "mkdir") != 0) {
            for (int i = 1; i < argc; i++) {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[i]);
            }
        }
        else {
            yflag = 1; // arugument required error
        }
    }
    // 'delete', 'rmdir' command
    else if (strcmp(argv[0], "delete") == 0 || strcmp(argv[0], "rmdir") == 0) {
        if (strcmp(argv[0], "delete") == 0) 
            strcpy(cmd_buff, "DELE");
        else
            strcpy(cmd_buff, "RMD");

        // option check for delete and rmdir
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        if (argc == 1)
            yflag = 1; // arugument required error
        else {
            for (int i = 1; i < argc; i++) {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[i]);
            }
        }
    }
    // 'rename' command
    else if (strcmp(argv[0], "rename") == 0) {
        // option check for rename
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "RNFR ");
        if (argc != 3)
            rflag = 1; // arugument required error
        else {
            strcat(cmd_buff, argv[1]);
            strcat(cmd_buff, " RNTO ");
            strcat(cmd_buff, argv[2]);
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
    if (rflag)
        strcpy(cmd_buff, "Error: r");
    else if (qflag)
        oflag ? strcpy(cmd_buff, "Error: o") : strcpy(cmd_buff, "Error: q");
    else if (oflag)
        strcpy(cmd_buff, "Error: o");
    else if (xflag)
        strcpy(cmd_buff, "Error: x");
    else if (yflag)
        strcpy(cmd_buff, "Error: y");
    
    printf("%s\n", cmd_buff);

    return 0;
}