//////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                //
// Date : 2024/05/12                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #2-3 ( ftp server )        //
// Description : The program work as Linux terminal commands ls, dir//
//              pwd, cd, mkdir, delete, rmdir, rename, quit command.//
//              This file work for client command parsing and       //
//              change to FTP command and pass it to server file    //
//////////////////////////////////////////////////////////////////////
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
#include <time.h>

#define BUF_SIZE 100000
#define MAX_BUF 20

int log_in(int sockfd); // Function prototype for log_in
void sh_int(int signo); // signal handler for SIGINT
void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag);
int conv_cmd(char *buff, char *cmd_buff);

int sockfd;

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
int main(int argc, char **argv)
{
    char buff[BUF_SIZE], cmd_buff[BUF_SIZE], rcv_buff[BUF_SIZE];
    struct sockaddr_in serv_addr;

    // Set up signal handlers
    if (signal(SIGINT, sh_int) == SIG_ERR)
    {
        perror("Cannot handle SIGINT");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        exit(1);
    }

    // Send IP and port to server
    char ip_port_str[2 * MAX_BUF + 1]; // Buffer to hold IP and port
    snprintf(ip_port_str, sizeof(ip_port_str), "%s\n%d", argv[1], htons(atoi(argv[2])));
    write(sockfd, ip_port_str, strlen(ip_port_str));

    // Receive connection result
    char buf[MAX_BUF];                  // Buffer for server response
    int n = read(sockfd, buf, MAX_BUF); // Read server response
    buf[n] = '\0';                      // Null-terminate string
    if (strncmp(buf, "REJECTION", strlen("REJECTION")) == 0)
    {
        write(STDOUT_FILENO, "This client can't access. Close the session.", sizeof("This client can’t access. Close the session."));
        close(sockfd);
        return 0;
    }
    else if (strncmp(buf, "ACCEPTED", strlen("ACCEPTED")) == 0)
    {
        char temp_output[256]; // temp buffer
        // current time
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Z %Y", t);
        sprintf(temp_output, "220 sswlab.kw.ac.kr FTP server (version myftp [1.0] %s) ready.\n", time_str);
        write(STDOUT_FILENO, temp_output, 256);
        memset(temp_output, 0, sizeof(temp_output));
    }

    if (log_in(sockfd) == 1)
    { // Proceed to log in
        close(sockfd);
        return 0;
    }

    while (1)
    {
        write(STDOUT_FILENO, "ftp> ", 6);
        read(STDIN_FILENO, buff, BUF_SIZE);

        if (conv_cmd(buff, cmd_buff) != 0)
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            write(sockfd, cmd_buff, strlen(cmd_buff));
            close(sockfd);
            exit(1);
        }

        int n = strlen(cmd_buff);
        if (write(sockfd, cmd_buff, n) != n)
        {
            write(STDERR_FILENO, "write() error!!\n", sizeof("write() error!!\n"));
            close(sockfd);
            exit(1);
        }

        if ((n = read(sockfd, rcv_buff, BUF_SIZE - 1)) < 0)
        {
            write(STDERR_FILENO, "read() error\n", sizeof("read() error\n"));
            close(sockfd);
            exit(1);
        }
        memset(buff, 0, BUF_SIZE);
        memset(cmd_buff, 0, BUF_SIZE);

        if (strncmp(rcv_buff, "QUIT", 4) == 0)
        {
            write(STDOUT_FILENO, "Program quit!!\n", strlen("Program quit!!\n"));
            close(sockfd);
            exit(0);
        }

        printf("%s\n", rcv_buff);
        memset(rcv_buff, 0, sizeof(rcv_buff));
    }

    close(sockfd);
    return 0;
}

int log_in(int sockfd)
{
    int n;
    char user[MAX_BUF], passwd[MAX_BUF], buf[MAX_BUF];

    while (1)
    {
        char buffer[MAX_BUF * 2 + 1];                              // Buffer to hold user and password
        printf("Input ID : ");                                     // Prompt for user ID
        fgets(user, MAX_BUF, stdin);                               // Read user ID
        user[strcspn(user, "\n")] = 0;                             // Remove newline character
        printf("Input Password : ");                               // Prompt for password
        fgets(passwd, MAX_BUF, stdin);                             // Read password
        passwd[strcspn(passwd, "\n")] = 0;                         // Remove newline character
        snprintf(buffer, MAX_BUF * 2 + 1, "%s\n%s", user, passwd); // Format user and password
        write(sockfd, buffer, MAX_BUF * 2 + 1);                    // Send to server

        n = read(sockfd, buf, MAX_BUF);
        buf[n] = '\0';
        if (strcmp(buf, "OK") == 0)
        {
            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';

            if (strncmp(buf, "230", strlen("230")) == 0)
            {
                write(STDOUT_FILENO, "230 User sslab logged in.\n", strlen("230 User sslab logged in.\n"));
                return 0;
            }
            else if (strncmp(buf, "430", strlen("430")) == 0)
            {
                write(STDOUT_FILENO, "430 Invalid username or password\n", strlen("430 Invalid username or password\n"));
            }
            else if (strncmp(buf, "530", strlen("530")) == 0)
            {
                write(STDOUT_FILENO, "530 Failed to log-in\n", strlen("530 Failed to log-in\n"));
                return 1;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// sh_int                                                           //
// =================================================================//
// Input: signo -> Signal number (SIGINT)                           //
//                                                                  //
// Output: None                                                     //
//                                                                  //
// Purpose: Signal handler for SIGINT. Handles interrupt signal by  //
//          sending a "QUIT" message to the server and then exits   //
//          the program gracefully.                                 //
//////////////////////////////////////////////////////////////////////
void sh_int(int signo)
{
    // Send the "QUIT" message to the server
    if (write(sockfd, "QUIT", strlen("QUIT")) < 0)
    {
        perror("Failed to send QUIT message");
    }
    // Exit the program
    exit(0);
}

//////////////////////////////////////////////////////////////////////
// parse_options                                                    //
// =================================================================//
// Input: argc -> Number of command line arguments                  //
//        argv -> Array of command line argument strings            //
//        aflag -> Pointer to the flag for option 'a'               //
//        lflag -> Pointer to the flag for option 'l'               //
//        oflag -> Pointer to the flag for invalid options          //
//                                                                  //
// Output: None                                                     //
//                                                                  //
// Purpose: Parses command line options and sets corresponding      //
//          flags based on the options provided. Flags 'a' and 'l'  //
//          are set if their respective options are found, and      //
//          oflag is set if an invalid option is encountered.       //
//////////////////////////////////////////////////////////////////////
void parse_options(int argc, char *argv[], int *aflag, int *lflag, int *oflag)
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        { // check if it is option
            for (int j = 1; argv[i][j] != '\0'; j++)
            {
                switch (argv[i][j])
                {
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
int conv_cmd(char *buff, char *cmd_buff)
{
    int aflag = 0, lflag = 0;                                  // flag for option management
    int oflag = 0, xflag = 0, yflag = 0, qflag = 0, rflag = 0; // flag for error management

    char *token;
    int argc = 0;
    char *argv[100];
    // parse data from client buffer
    token = strtok(buff, " \n");
    while (token != NULL)
    {
        argv[argc++] = token;        // Assign the token to argv and increment argc
        token = strtok(NULL, " \n"); // Continue tokenizing the string
    }
    argv[argc] = NULL;

    // If first argument is "ls"
    if (strcmp(argv[0], "ls") == 0)
    {
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
        char *temp = argv[argc - 1];
        if (strcmp(temp, "ls") != 0 && temp[0] != '-')
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, temp); // store path to temp
            if (argc > 2)
                if (strcmp(argv[argc - 2], "ls") != 0 && argv[argc - 2][0] != '-')
                    xflag = 1; // set too many argument error flag
        }
        else if (strcmp(temp, "ls") == 0 && argc > 1)
        {
            strcat(cmd_buff, " ");
            strcat(cmd_buff, temp);
        }
    }
    // 'dir' command
    else if (strcmp(argv[0], "dir") == 0)
    {
        // option check for dir
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // 'dir' command does not take any options, only a path
        strcpy(cmd_buff, "LIST");

        if (strcmp(argv[argc - 1], "dir") != 0)
        {
            if (strcmp(argv[argc - 2], "dir") == 0)
            {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[argc - 1]); // store path to buffer
            }
            else
                qflag = 1;
        }
    }
    // 'pwd' command
    else if (strcmp(argv[0], "pwd") == 0)
    {
        // option check for pwd
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // 'pwd' command does not take any option and path
        strcpy(cmd_buff, "PWD");

        if (strcmp(argv[argc - 1], "pwd") != 0)
        {
            qflag = 1;
        }
    }
    // 'cd' command
    else if (strcmp(argv[0], "cd") == 0)
    {
        // option check for cd
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "CWD");
        if (strcmp(argv[argc - 1], "cd") != 0)
        {
            if (strcmp(argv[argc - 2], "cd") == 0)
            {
                if (strcmp(argv[argc - 1], "..") == 0)
                    strcpy(cmd_buff, "CDUP");
                else
                {
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
    else if (strcmp(argv[0], "mkdir") == 0)
    {
        // option check for mkdir
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "MKD");

        if (strcmp(argv[argc - 1], "mkdir") != 0)
        {
            for (int i = 1; i < argc; i++)
            {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[i]);
            }
        }
        else
        {
            yflag = 1; // arugument required error
        }
    }
    // 'delete', 'rmdir' command
    else if (strcmp(argv[0], "delete") == 0 || strcmp(argv[0], "rmdir") == 0)
    {
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
        else
        {
            for (int i = 1; i < argc; i++)
            {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[i]);
            }
        }
    }
    // 'rename' command
    else if (strcmp(argv[0], "rename") == 0)
    {
        // option check for rename
        parse_options(argc, argv, &aflag, &lflag, &oflag);
        if (aflag == 1 || lflag == 1)
            oflag = 1;

        // append srv command to buffer
        strcpy(cmd_buff, "RNFR ");
        if (argc != 3)
            rflag = 1; // arugument required error
        else
        {
            strcat(cmd_buff, argv[1]);
            strcat(cmd_buff, " RNTO ");
            strcat(cmd_buff, argv[2]);
        }
    }
    // 'quit' command
    else if (strcmp(argv[0], "quit") == 0)
    {
        // 'quit' command does not take any options or path
        strcpy(cmd_buff, "QUIT");
        if (argc != 1)
            qflag = 1;
    }
    // Unknown command
    else
    {
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