#include <stdio.h>      // For printf, scanf functions
#include <stdlib.h>     // For atoi function
#include <string.h>     // For strcmp function
#include <unistd.h>     // For read, write, close functions
#include <sys/types.h>  // For data type definitions
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For structure definitions
#include <arpa/inet.h>  // For inet_pton function

#define MAX_BUF 20

int main(int argc, char *argv[]) {
    if (argc != 3)
        exit(EXIT_FAILURE);

    int sockfd, n, p_pid;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("**It is connected to Server**\n");

    log_in(sockfd);
    close(sockfd);
    return 0;
}

void log_in(int sockfd) {
    int n;
    char user[MAX_BUF], *passwd, buf[MAX_BUF];

    read(sockfd, buf, MAX_BUF);
    if (strcmp(buf, "REJECTION") == 0) {
        printf("**Connection refused**\n", user);
        return 0;
    } else if (strcmp(buf, "ACCEPTED") == 0) {
        while(1) {
            printf("Enter username: ");
            scanf("%s", user);
            printf("Enter password: ");
            scanf("%s", passwd);
            snprintf(buf, MAX_BUF, "%s\n%s", user, passwd);
            
            write(sockfd, buf, MAX_BUF);

            n = read(sockfd, buf, MAX_BUF);
            buf[n] = '\0';
            if(!strcmp(buf, "OK")) {
                n = read(sockfd, buf, MAX_BUF);
                buf[n] = '\0';

                if (strcmp(buf, "OK") == 0) {
                    printf("** User '%s' logged in **\n", user);
                } else if (strcmp(buf, "FAIL") == 0) {
                    printf("** Log-in failed **\n");
                } else if (strcmp(buf, "DISCONNECTION") == 0) {
                    printf("** Connection closed **\n");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        printf("**Unknown command**\n", user);
    }
}


