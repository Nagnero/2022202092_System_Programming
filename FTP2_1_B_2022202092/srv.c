#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define MAX_BUFF 10000
#define RCV_BUFF 10000
#define SEND_BUFF 10000
#define SERVER_ADDR "127.0.0.1"
#define PORT 13428

int socketConnection();
int cmd_process(char* buff, char* result_buff);

//////////////////////////////////////////////////////////////////////
// compare_strings                                                  //
// =================================================================//
// Input: a -> pointer to the first string's pointer                //
//        b -> pointer to the second string's pointer               //
//                                                                  //
// Output: int - Negative if 'a' is less than 'b',                  //
//               zero if 'a' and 'b' are equal,                     //
//               positive if 'a' is greater than 'b'.               //
//                                                                  //
// Purpose: Compare two strings for use in sorting functions.       //
//          This function is intended to be used as a comparator    //
//          in sorting algorithms, especially with qsort.           //
//////////////////////////////////////////////////////////////////////
int compare_strings(const void *a, const void *b) {
    const char *str_a = *(const char **)a;
    const char *str_b = *(const char **)b;
    return strcmp(str_a, str_b);
}

//////////////////////////////////////////////////////////////////////
// print_file_info                                                  //
// =================================================================//
// Input: buf -> pointer to a struct stat containing file info      //
//        name -> the name of the file or directory to print info   //
//                                                                  //
// Output: None (void function)                                     //
//                                                                  //
// Purpose: Print detailed information about a file or directory,   //
//          similar to the 'ls -l' command in Linux. This includes  //
//          file permissions, number of links, owner name, group    //
//          name, file size, last modification date, and the file   //
//          or directory name.                                      //
//          This function also appends the formatted information    //
//          to a global output string.                              //
//////////////////////////////////////////////////////////////////////
void print_file_info(struct stat *buf, char *name, char* output) {
    char perms[11];
    sprintf(perms, "%c%c%c%c%c%c%c%c%c%c",
            S_ISDIR(buf->st_mode) ? 'd' : '-',
            buf->st_mode & S_IRUSR ? 'r' : '-',
            buf->st_mode & S_IWUSR ? 'w' : '-',
            buf->st_mode & S_IXUSR ? 'x' : '-',
            buf->st_mode & S_IRGRP ? 'r' : '-',
            buf->st_mode & S_IWGRP ? 'w' : '-',
            buf->st_mode & S_IXGRP ? 'x' : '-',
            buf->st_mode & S_IROTH ? 'r' : '-',
            buf->st_mode & S_IWOTH ? 'w' : '-',
            buf->st_mode & S_IXOTH ? 'x' : '-');

    char temp1[1024];

    struct passwd *pwd = getpwuid(buf->st_uid);
    struct group *grp = getgrgid(buf->st_gid);
    char date[20];
    strftime(date, sizeof(date), "%b %d %H:%M", localtime(&buf->st_mtime));

    sprintf(temp1, "%s %3ld %s %s %6ld %s %s\n", 
            perms, buf->st_nlink, pwd->pw_name, grp->gr_name, buf->st_size, date, name);
    strcat(output, temp1);
}


int main(int argc, char **argv)
{
    char buff[MAX_BUFF], result_buff[SEND_BUFF];
    int sockfd = socketConnection();
    int n, clientfd;
    struct sockaddr_in client;

    while(sockfd != -1) {
        bzero((char*)&client, sizeof(client)); // initialize client_addr
        socklen_t client_len = sizeof(client);
        clientfd = accept(sockfd, (struct sockaddr *)&client, &client_len);

        if(clientfd < 0) {
            write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
            close(sockfd);
            break;
        }

        /* display client ip and port */
        printf("==========Client info==========\n");
        printf("client IP: %s\n\n", SERVER_ADDR);
        printf("client port: %d\n", PORT);
        printf("===============================\n");

        // write(1, "path : ", 7);
        // write(1, path, strlen(path));
        // write(1, "\n", 1);
            
        while ((n = read(clientfd, buff, MAX_BUFF)) > 0) {
            printf("%s\n", buff);
            buff[n] = '\0';

            if (cmd_process(buff, result_buff) < 0) {
                write(STDERR_FILENO, "cmd_process() err!!\n", sizeof("cmd_process() err!!\n"));
                break;
            }

            if (strncmp(result_buff, "QUIT", 4) == 0) {
                write(clientfd, result_buff, strlen(result_buff));
                //write(STDOUT_FILENO, "QUIT", sizeof("QUIT"));
                printf("\n");
                close(clientfd);
                close(sockfd);
                exit(0);
            }
            else {
                //printf("%s\n", buff);
                write(clientfd, result_buff, strlen(result_buff));
            }

            memset(result_buff, 0, sizeof(result_buff));
            memset(buff, 0, sizeof(buff));
        }
        if (clientfd != 0)
            close(clientfd);
    }
    close(sockfd);
    return 0;
}



int socketConnection(){ 
///////////////////////// socket start /////////////////////////
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    bzero((char*)&server, sizeof(server)); // initialize server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    server.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind");
        close(sockfd); 
        exit(1);
    }
    if (listen(sockfd, 5)) {
        perror("listen");
        exit(1);
    }
    
    return sockfd; // retrun socket file descripter
}




int cmd_process(char* buff, char*result_buff) {
    int argc = 0;
    char* argv[64];
    char* token;
    char path[1024];

    // parse data from client buffer
    token = strtok(buff, " \n");
    while (token != NULL) {
        argv[argc++] = token; // Assign the token to argv and increment argc
        token = strtok(NULL, " \n"); // Continue tokenizing the string
    }
    argv[argc] = NULL;

    // printing error state
    if (strcmp(argv[0], "Error:") == 0) {
        if (argv[1][0] == '!')
            strcpy(result_buff, "Error: unknown command\n");
        else if (argv[1][0] == 'q')
            strcpy(result_buff, "Error: arguments is not required\n");
        else if (argv[1][0] == 'o')
            strcpy(result_buff, "Error: invalid option\n");
        else if (argv[1][0] == 'x')
            strcpy(result_buff, "Error: too many argument\n");
    }
    else if (strcmp(argv[0], "NLST") == 0) {
        // argv[1]: option, argv[2]: path
        DIR *dp; // Directory stream
        struct dirent *dirp; // Pointer for directory entry
        struct stat buf;
        char* temp[64];
        int index = 0;

        // strcat(result_buff, argv[0]);

        if (argc == 1) {
            strcat(result_buff, "\n");
            // open directory with path
            dp = opendir(".");

            while ((dirp = readdir(dp)) != NULL) {                
                if (dirp->d_name[0] != '.') {
                    // Build the full path to the file
                    char path[1024];
                    snprintf(path, sizeof(path), "./%s", dirp->d_name);

                    // Get file status
                    if (stat(path, &buf) == 0) {
                        // Check if it is a directory
                        if (S_ISDIR(buf.st_mode)) {
                            // Append '/' to directory name
                            temp[index] = dirp->d_name;
                            strcat(temp[index++], "/");
                        }
                        else {
                            temp[index++] = dirp->d_name;                        
                        }
                    }
                }
            }

            // Read directories in a loop
            while ((dirp = readdir(dp)) != NULL) {
                strcat(result_buff, dirp->d_name); // Print each directories' name
            }

            // sort buffer strings before print
            qsort(temp, index, sizeof(char *), compare_strings);
            int offset = strlen(result_buff);
            for (int i = 0; i < index; i++) {
                offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "%-25s", temp[i]);
                if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                    offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "\n");
                }
            }
        }
        else if (strcmp(argv[1], "-a") == 0) {
            // strcat(result_buff, " -a\n");

            // open directory with path
            if (argc == 3) 
                dp = opendir(argv[2]);
            else
                dp = opendir(".");

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                switch (errno) {
                    case ENOENT:
                        strcat(result_buff, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(result_buff, "Error: cannot access\n");
                        break;
                    default:
                        strcat(result_buff, "Error: no such file or directory\n");
                }
                return -1;
            }
            
            if (dp != NULL) {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (argc == 3)
                        snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                    else
                        snprintf(path, sizeof(path), "./%s", dirp->d_name);

                    // Get file status
                    if (stat(path, &buf) == 0) {
                        // Check if it is a directory
                        if (S_ISDIR(buf.st_mode)) {
                            // Append '/' to directory name
                            temp[index] = dirp->d_name;
                            strcat(temp[index++], "/");
                        }
                        else {
                            temp[index++] = dirp->d_name;                        
                        }
                    }               
                }

                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                int offset = strlen(result_buff);
                for (int i = 0; i < index; i++) {
                    offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "%-25s", temp[i]);
                    if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                        offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "\n");
                    }
                }
            }
        }
        else if (strcmp(argv[1], "-l") == 0) {
            // strcat(result_buff, " -l\n");

            // open directory with path
            if (argc == 3) 
                dp = opendir(argv[2]);
            else
                dp = opendir(".");

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                switch (errno) {
                    case ENOENT:
                        strcat(result_buff, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(result_buff, "Error: cannot access\n");
                        break;
                    default:
                        strcat(result_buff, "Error: no such file or directory\n");
                }
                return -1;
            }

            if (dp != NULL) {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (argc != 3)
                        snprintf(path, sizeof(path), "./%s", dirp->d_name);
                    else
                        snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                    

                    // Get file status
                    if (stat(path, &buf) == 0) {
                        // Check if it is a directory
                        if (S_ISDIR(buf.st_mode)) {
                            // Append '/' to directory name
                            temp[index] = dirp->d_name;
                            strcat(temp[index++], "/");
                        }
                        else {
                            temp[index++] = dirp->d_name;                        
                        }
                    }               
                }

                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                
                for (int i = 0; i < index; i++) {
                    if (temp[i][0] == '.') continue;
                    if (argc == 3)
                        snprintf(path, sizeof(path), "./%s/%s", argv[2], temp[i]);
                    else
                        snprintf(path, sizeof(path), "./%s", temp[i]);
                    if (stat(path, &buf) == 0) {
                        print_file_info(&buf, temp[i], result_buff);
                    }
                }
            }
        }
        else if (strcmp(argv[1], "-al") == 0) {
            // strcat(result_buff, " -al\n");

            // open directory with path
            if (argc == 3) 
                dp = opendir(argv[2]);
            else
                dp = opendir(".");

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                switch (errno) {
                    case ENOENT:
                        strcat(result_buff, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(result_buff, "Error: cannot access\n");
                        break;
                    default:
                        strcat(result_buff, "Error: no such file or directory\n");
                }
                return -1;
            }

            if (dp != NULL) {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (argc == 3)
                        snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                    else
                        snprintf(path, sizeof(path), "./%s", dirp->d_name);
                    
                    // Get file status
                    if (stat(path, &buf) == 0) {
                        // Check if it is a directory
                        if (S_ISDIR(buf.st_mode)) {
                            // Append '/' to directory name
                            temp[index] = dirp->d_name;
                            strcat(temp[index++], "/");
                        }
                        else {
                            temp[index++] = dirp->d_name;                        
                        }
                    }               
                }

                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                
                for (int i = 0; i < index; i++) {
                    if (argc == 3)
                        snprintf(path, sizeof(path), "./%s/%s", argv[2], temp[i]);
                    else
                        snprintf(path, sizeof(path), "./%s", temp[i]);
                    if (stat(path, &buf) == 0) {
                        print_file_info(&buf, temp[i], result_buff);
                    }
                }
            }
        }
        // given path without option
        else {
            // strcat(result_buff, "\n");

            // open directory with path
            dp = opendir(argv[1]);

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                switch (errno) {
                    case ENOENT:
                        strcat(result_buff, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(result_buff, "Error: cannot access\n");
                        break;
                    default:
                        strcat(result_buff, "Error: no such file or directory\n");
                }
                return -1;
            }
            
            if (dp != NULL) {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    snprintf(path, sizeof(path), "./%s/%s", argv[1], dirp->d_name);

                    // Get file status
                    if (stat(path, &buf) == 0) {
                        // Check if it is a directory
                        if (S_ISDIR(buf.st_mode)) {
                            // Append '/' to directory name
                            temp[index] = dirp->d_name;
                            strcat(temp[index++], "/");
                        }
                        else {
                            temp[index++] = dirp->d_name;                        
                        }
                    }               
                }

                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                int offset = strlen(result_buff);
                for (int i = 0; i < index; i++) {
                    offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "%-25s", temp[i]);
                    if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                        offset += snprintf(result_buff + offset, sizeof(result_buff) - offset, "\n");
                    }
                }
            }
        }

        strcat(result_buff, "\n");
        if (dp != NULL)
            closedir(dp); // Close the directory stream
        
        return 0;
    }
    // 'QUIT' command
    else if (strcmp(argv[0], "QUIT") == 0) {
        strcpy(result_buff, "QUIT\n");
        return 0;
    }
    else
        return -1;

    return 0;
}