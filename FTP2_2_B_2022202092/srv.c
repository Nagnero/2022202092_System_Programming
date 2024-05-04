#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/wait.h> 
#include <signal.h>

#define BUF_SIZE 256

void sh_chld(int); // signal handler for SIGCHLD 
void sh_alrm(int); // signal handler for SIGALRM

int main(int argc, char **argv) {
    char buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr; 
    int server_fd, client_fd;
    socklen_t len;

    /* Applying signal handler(sh_alrm) for SIGALRM */
    signal(SIGALRM, sh_alrm);
    /* Applying signal handler(sh_chld) for SIGCHLD */
    signal(SIGCHLD, sh_chld);

    server_fd = socket (PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl (INADDR_ANY); 
    server_addr.sin_port = htons (atoi(argv[1]));

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(server_fd, 5);

    while(1) {
        pid_t pid;
        len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);

        if ((pid = fork()) < 0) {
            printf("fork err");
        }
        else if (pid == 0) { // child process
            if(client_fd < 0) {
                write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
                close(client_fd);
                break;
            }            

            while ((n = read(client_fd, buff, BUF_SIZE)) > 0) {
                for (int i = 0 ; i < strlen(buff); i++) {
                    if (buff[i] == '\n') {
                        while (i < strlen(buff)) {
                            buff[++i] = '\0';
                        }
                    }
                }

                if (strcmp(buff, "QUIT\n") == 0) {
                    alarm(1);
                    break;
                }
                
                write(client_fd, buff, BUF_SIZE);
            }
        }
        else { 
            char output[BUF_SIZE] = {};
            /* display client ip and port */
            char temp_output[1000];
            snprintf(temp_output, sizeof(temp_output), 
                    "==========Client info==========\n"
                    "client IP: %s\n\n"
                    "client port: %d\n"
                    "===============================\n",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            write(STDOUT_FILENO, temp_output, strlen(temp_output));
            sprintf(output,  "Child Process ID : %d\n", pid);
            write(STDOUT_FILENO, output, BUF_SIZE);
        }

        close(client_fd);
    }
    return 0;
}

void sh_chld(int signum) {
    printf("Status of Child process was changed.\n"); 
    wait(NULL);
}

void sh_alrm(int signum) {
    printf("Child Process (PID : %d) will be terminated.\n", getpid()); 
    exit(1);
}