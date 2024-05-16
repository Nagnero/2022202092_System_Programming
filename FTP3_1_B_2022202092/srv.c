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
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    FILE *fp_checkIP; // FILE stream to check client’s IP
    char buffer[MAX_BUF];

    socklen_t clilen = sizeof(cliaddr);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(atoi(argv[1]));

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

        // Check if client's IP is allowed to access
        if (!is_allowed(cliaddr.sin_addr)) {
            printf("** Access Denied for IP: %s **\n", inet_ntoa(cliaddr.sin_addr));
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
}


int log_auth(int connfd) {
    char user[MAX_BUF], passwd[MAX_BUF];
    int n, count = 1;

    while (1) {
        /* 코드 작성 (hint: username과 password를 client로부터 받는다) */
        n = read(connfd, user, MAX_BUF);
        user[n] = '\0'; // Null terminate the string

        n = read(connfd, passwd, MAX_BUF);
        passwd[n] = '\0'; // Null terminate the string

        write(connfd, "OK", MAX_BUF);
        if ((n = user_match(user, passwd)) == 1) {
            write(connfd, "OK", 3);
            return 1;
        }
        else if (n == 0)
        {
            if (count >= 3)
            {
                /* 코드 작성 (hint: 3 times fail) */
            }
            write(connfd, "FAIL", MAX_BUF);
            count++;
            continue;
        }
    }
    return 1;
}

int user_match(char *user, char *passwd)
{
    FILE *fp;
    struct passwd *pw;
    fp = fopen("passwd", "r");
    /* 코드 작성 (hint: 인증 성공 시 return 1, 인증 실패 시 return 0 */
}
