#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/getopt_core.h>

#define MAX_BUFF 10000
#define RCV_BUFF 10000
#define PORT 13428

int conv_cmd(char *buff, char *cmd_buff);

void main(int argc, char **argv)
{
    char buff[MAX_BUFF], cmd_buff[MAX_BUFF], rcv_buff[RCV_BUFF];
    int n;

    ///////////////////////// socket start /////////////////////////
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    memset((char *)&server, '\0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)))
    {
        perror("connect");
        exit(1);
    }
    ///////////////////////// socket ends /////////////////////////

    /* open socket and connect to server */
    for (;;)
    {
        /* convert ls (including options) to NLST (including options) */
        if (conv_cmd(buff, cmd_buff) < 0)
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n", strlen("conv_cmd() error!!\n"));
            exit(1);
        }

        if (send(sockfd, cmd_buff, strlen(cmd_buff) + 1, 0) == -1)
        {
            perror("send");
            exit(1);
        }

        if (recv(sockfd, rcv_buff, sizeof(rcv_buff), 0) == -1)
        {
            perror("recv");
            exit(1);
        }
        n = strlen(cmd_buff);
        if (write(sockfd, cmd_buff, n) != n)
        {
            write(STDERR_FILENO, "write() error!!\n", strlen("write() error!!\n"));
            exit(1);
        }
        if ((n = read(sockfd, rcv_buff, RCV_BUFF - 1)) < 0)
        {
            write(STDERR_FILENO, "read() error\n", strlen("read() error\n"));
            exit(1);
        }

        rcv_buff[n] = '\0';
        if (!strcmp(rcv_buff, "QUIT"))
        {
            write(STDOUT_FILENO, "Program quit!!\n", strlen("Program quit!!\n"));
            exit(1);
        }
        process_result(rcv_buff);
        /*display ls(including options) command result */
    }
}

int conv_cmd(char *buff, char *cmd_buff)
{
    char c;                              // Variable to store the option character returned by getopt
    int aflag = 0, lflag = 0;            // flag for option management
    int oflag = 0, xflag = 0, qflag = 0; // flag for error management

    char *token;
    int argc = 1;
    char *argv[100];
    // parse data from client buffer
    token = strtok(buff, " ");
    while (token != NULL)
    {
        argv[argc++] = token;      // Assign the token to argv and increment argc
        token = strtok(NULL, " "); // Continue tokenizing the string
    }
    argv[argc--] = NULL;

    opterr = 0; // Disable automatic error reporting by getopt

    if (strcmp(argv[1], "ls") == 0)
    {
        // option check for ls
        while ((c = getopt(argc, argv, "al")) != -1)
        {
            // Switch statement to handle the option character returned by getopt()
            switch (c)
            {
            case 'a':      // If option 'a' is given
                aflag = 1; // Set 'aflag' as 1
                break;
            case 'l':      // If option 'b' is given
                lflag = 1; // Set 'bflag' as 1
                break;
            case '?': // If an unrecognized option is encountered
                oflag = 1;
            }
        }

        strcpy(cmd_buff, "NLST"); // Start building the NLST command

        // Append appropriate flags to the command
        if (aflag == 1 && lflag == 0)
            strcat(cmd_buff, " -a");
        else if (aflag == 0 && lflag == 1)
            strcat(cmd_buff, " -l");
        else if (aflag == 1 && lflag == 1)
            strcat(cmd_buff, " -al");

        // Append path if specified
        if (strcmp(argv[argc - 1], "ls") != 0)
        {
            if (strcmp(argv[argc - 2], "ls") == 0)
            {
                strcat(cmd_buff, " ");
                strcat(cmd_buff, argv[argc - 1]); // store path to temp
            }
            else
                xflag = 1; // set error flag x: too many argument
        }
    }
    // 'quit' command
    else if (strcmp(argv[1], "quit") == 0)
    {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1)
        {
            // Switch statement to handle the option character returned by getopt()
            switch (c)
            {
            case '?': // If an unrecognized option is encountered
                oflag = 1;
            }
        }

        // 'quit' command does not take any options or path
        strcpy(cmd_buff, "QUIT");
        if (argc != 2)
            qflag = 1;
    }
    // Unknown command
    else
    {
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
}