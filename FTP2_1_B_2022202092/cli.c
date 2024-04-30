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

#define MAX_BUFF 100000
#define RCV_BUFF 100000

int socketConnection(char* server_addr, int port);
int conv_cmd(char *buff, char *cmd_buff);

void process_result(char *result);

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
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            write(sockfd, cmd_buff, strlen(cmd_buff));
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
        write(STDOUT_FILENO, cmd_buff, 1024);
        return 1;
    }

    // process error flag
    if (qflag)
        oflag ? strcpy(cmd_buff, "Error: o") : strcpy(cmd_buff, "Error: q");
    else if (oflag)
        strcpy(cmd_buff, "Error: o");
    else if (xflag)
        strcpy(cmd_buff, "Error: x");
    else
        return 0;
    
    return 1;
}

void process_result(char *result) {
    printf("%s\n", result);
    memset(result, 0, sizeof(*result));
}


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