//////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                //
// Date : 2024/05/18                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #3-1 (ftp server)          //
// Description : This file is part of a server application that     //
//               listens for connections, authenticates clients,    //
//               and handles user commands.                         //
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define MAX_BUF 1024 * 1024
#define SERVER_ADDR "127.0.0.1"

int socketConnection(int port);
char* convert_str_to_addr(char *str, unsigned int *port);
void handle_client(int sockfd_control);
int cmd_process(char* buff, char* result_buff);
int compare_strings(const void *a, const void *b);
void print_file_info(struct stat *buf, char *name, char* output);

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: argc -> Number of command line arguments                  //
//        argv -> Array of command line argument strings            //
//                                                                  //
// Output: int - Returns 0 for normal termination                   //
//               Returns 1 for error termination                    //
//                                                                  //
// Purpose: Initializes the server to listen on a specified port,   //
//          accepts client connections, and handles client          //
//          authentication and command execution.                   //
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *host_ip;
    char temp[25] = "127,0,0,1,184,75";
    unsigned int port_num;

    // control connection
    char buff[MAX_BUF], result_buff[MAX_BUF];
    int port = atoi(argv[1]);
    int sockfd_control = socketConnection(port);
    int n, clientfd;
    struct sockaddr_in client;

    while(sockfd_control != -1) {
        bzero((char*)&client, sizeof(client)); // initialize client_addr
        socklen_t client_len = sizeof(client);
        clientfd = accept(sockfd_control, (struct sockaddr *)&client, &client_len);

        if(clientfd < 0) {
            write(STDERR_FILENO, "client_info() err!!\n", sizeof("client_info() err!!\n"));
            close(sockfd_control);
            break;
        }
        

        while ((n = read(clientfd, temp, MAX_BUF)) > 0) {
            printf("ASF%s\n",temp);
            // open data socket
            host_ip = convert_str_to_addr(temp, &port_num);

            // 데이터 소켓 연결 부분 추가
            int sockfd_data = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_data < 0) {
                perror("socket creation failed");
                continue;
            }

            struct sockaddr_in data_addr;
            memset(&data_addr, 0, sizeof(data_addr));
            data_addr.sin_family = AF_INET;
            inet_pton(AF_INET, host_ip, &data_addr.sin_addr);
            data_addr.sin_port = htons(port_num);

            if (connect(sockfd_data, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
                perror("data connection failed");
                close(sockfd_data);
                free(host_ip);
                continue;
            }



            if (cmd_process(buff, result_buff) < 0) {
                write(STDERR_FILENO, "cmd_process() err!!\n", sizeof("cmd_process() err!!\n"));
                write(STDERR_FILENO, result_buff, strlen(result_buff));
                write(clientfd, result_buff, strlen(result_buff));
                memset(result_buff, 0, sizeof(result_buff));
                memset(buff, 0, sizeof(buff));
                continue;
            }

            

            if (strncmp(result_buff, "QUIT", 4) == 0) {
                write(clientfd, result_buff, strlen(result_buff));
                printf("\n");
                close(clientfd);
                close(sockfd_data);
                close(sockfd_control);
                free(host_ip);
                exit(0);
            } else {
                write(clientfd, result_buff, strlen(result_buff));
                write(sockfd_data, result_buff, strlen(result_buff));
            }

            memset(result_buff, 0, sizeof(result_buff));
            memset(buff, 0, sizeof(buff));

            close(sockfd_data);  // 데이터 소켓 닫기
            free(host_ip);
        }
        if (clientfd != 0)
            close(clientfd);
    }

    close(sockfd_control);
    return 0;
}

//////////////////////////////////////////////////////////////////////
// socketConnection                                                 //
// =================================================================//
// Input: port -> Integer specifying the port number to bind the    //
//                socket                                            //
//                                                                  //
// Output: int - Socket file descriptor if successful, -1 on failure//
//                                                                  //
// Purpose: Sets up and returns a socket bound to the specified     //
//          port on the server's local address. It is responsible   //
//          for creating the socket, binding it, and setting it up  //
//          to listen for incoming connections.                     //                                         //
//////////////////////////////////////////////////////////////////////
int socketConnection(int port) {
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
    server.sin_port = htons(port);

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


//client로부터 받은 PORT명령어에 붙은 IP주소와 포트번호를 변경
char* convert_str_to_addr(char *str, unsigned int *port) { 
    unsigned int ip_parts[4], port_parts[2];
    char *addr = malloc(INET_ADDRSTRLEN);
    if (addr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    sscanf(str, "%u,%u,%u,%u,%u,%u", &ip_parts[0], &ip_parts[1], &ip_parts[2], &ip_parts[3], &port_parts[0], &port_parts[1]);

    snprintf(addr, INET_ADDRSTRLEN, "%u.%u.%u.%u", ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3]);
    *port = (port_parts[0] << 8) | port_parts[1];

    return addr;
}


//////////////////////////////////////////////////////////////////////
// cmd_process                                                      //
// =================================================================//
// Input: buff -> Pointer to the input buffer containing the command//
//                string received from the client                    //
//        result_buff -> Pointer to the buffer where the result or  //
//                       error message will be stored                //
//                                                                  //
// Output: int - Returns 0 on successful processing, -1 on errors   //
//                                                                  //
// Purpose: Processes commands received from the client. This       //
//          function interprets the command, checks for valid       //
//          options and paths, and formats a response or error      //
//          message accordingly. It handles commands such as 'NLST',//
//          'QUIT', The function also manages directory             //
//          traversal and permissions, ensuring responses are       //
//          accurate to the file system state and access rights.    //
//////////////////////////////////////////////////////////////////////
int cmd_process(char* buff, char*result_buff) {
    int argc = 0;
    char* argv[64];
    char* token;

    // parse data from client buffer
    token = strtok(buff, " \n");
    while (token != NULL) {
        argv[argc++] = token; // Assign the token to argv and increment argc
        token = strtok(NULL, " \n"); // Continue tokenizing the string
    }
    argv[argc] = NULL;

    // printing error state
    if (strncmp(buff, "Error:", 6) == 0) {
        if (argv[1][0] == '!')
            strcpy(result_buff, "Error: unknown command\n\n");
        else if (argv[1][0] == 'q')
            strcpy(result_buff, "Error: arguments is not required\n\n");
        else if (argv[1][0] == 'o')
            strcpy(result_buff, "Error: invalid option\n\n");
        else if (argv[1][0] == 'x')
            strcpy(result_buff, "Error: too many argument\n\n");
        
        return -1;
    }
    else if (strcmp(argv[0], "NLST") == 0) {
        // argv[1]: option, argv[2]: path
        DIR *dp; // Directory stream
        struct dirent *dirp; // Pointer for directory entry
        struct stat buf;
        char* temp[10000];
        int index = 0;

        if (argc == 1) {
            // open directory with current path
            dp = opendir(".");

            while ((dirp = readdir(dp)) != NULL) {           
                if (dirp->d_name[0] != '.') {
                    // Build the full path to the file
                    char path[1024];
                    snprintf(path, sizeof(path), "./%s", dirp->d_name);

                    // Get file status
                    if (lstat(path, &buf) == 0) {
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
            
            // sort buffer strings before print
            qsort(temp, index, sizeof(char *), compare_strings);
            for (int i = 0; i < index; i++) {
                strcat(result_buff, temp[i]);
                strcat(result_buff, "\n");
            }
        }
        else if (strcmp(argv[1], "-a") == 0) {
            char temp_path[1024];
            int ab_flag = 0;
            // open directory with path
            if (argc == 3) {
                if (argv[2][0]== '/') {
                    strcpy(temp_path, argv[2]);
                    ab_flag = 1;
                }
                else
                    snprintf(temp_path, sizeof(temp_path), "./%s", argv[2]);
            }
            else
                strcpy(temp_path, ".");

            dp = opendir(temp_path);

            // Error handling for opendir
            if (dp == NULL) {
                if (errno == ENOTDIR) {
                    strcat(result_buff, argv[2]);
                    strcat(result_buff, "\n");
                    return 0;
                }
                else if (errno == EACCES) 
                    strcat(result_buff, "Error: Permission denied\n\n");
                else 
                    strcpy(result_buff, "Error: no such file exist\n\n");
                
                return -1;
            }
            else {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (argc == 3) {
                        if (ab_flag)
                            snprintf(path, sizeof(path), "%s/%s", argv[2], dirp->d_name);
                        else
                            snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                    }
                    else
                        snprintf(path, sizeof(path), "./%s", dirp->d_name);

                    // Get file status
                    if (lstat(path, &buf) == 0) {
                        // check read permission
                        if (access(path, R_OK) != -1) {
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

                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                for (int i = 0; i < index; i++) {
                    strcat(result_buff, temp[i]);
                    strcat(result_buff, "\n");
                }
            }
        }
        else if (strcmp(argv[1], "-l") == 0) {
            char temp_path[1024];
            int ab_flag = 0;
            // open directory with path
            if (argc == 3) {
                if (argv[2][0]== '/') {
                    strcpy(temp_path, argv[2]);
                    ab_flag = 1;
                }
                else
                    snprintf(temp_path, sizeof(temp_path), "./%s", argv[2]);
            }
            else
                strcpy(temp_path, ".");

            dp = opendir(temp_path);

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                if (errno == ENOTDIR) {
                    if (lstat(temp_path, &buf) == 0) {
                        print_file_info(&buf, argv[2], result_buff);
                    }
                    return 0;
                }
                else if (errno == EACCES) 
                    strcat(result_buff, "Error: Permission denied\n\n");
                else 
                    strcpy(result_buff, "Error: no such file exist\n\n");
                
                return -1;
            }
            else {                
                while ((dirp = readdir(dp)) != NULL) {   
                    if (dirp->d_name[0] != '.') {             
                        // Build the full path to the file
                        char path[1024];
                        if (argc == 3) {
                            if (ab_flag) 
                                snprintf(path, sizeof(path), "%s/%s", argv[2], dirp->d_name);
                            else
                                snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                        }
                        else
                            snprintf(path, sizeof(path), "./%s", dirp->d_name);

                        // Get file status
                        if (lstat(path, &buf) == 0) {
                            
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

                if (index != 0) {
                    // sort buffer strings before print
                    qsort(temp, index, sizeof(char *), compare_strings);
                    char path[1024];
                    for (int i = 0; i < index; i++) {
                        if (temp[i][0] == '.') continue;
                        if (argc == 3) {
                            if (ab_flag)
                                snprintf(path, sizeof(path), "%s/%s", argv[2], temp[i]);
                            else
                                snprintf(path, sizeof(path), "./%s/%s", argv[2], temp[i]);
                        }
                        else
                            snprintf(path, sizeof(path), "./%s", temp[i]);

                        if (lstat(path, &buf) == 0) {
                            print_file_info(&buf, temp[i], result_buff);
                        }
                    }
                }
                else
                    strcpy(result_buff, "total 0\n");                
            }
        }
        else if (strcmp(argv[1], "-al") == 0) {
            char temp_path[1024];
            int ab_flag = 0;
            // open directory with path
            if (argc == 3) {
                if (argv[2][0]== '/') {
                    strcpy(temp_path, argv[2]);
                    ab_flag = 1;
                }
                else
                    snprintf(temp_path, sizeof(temp_path), "./%s", argv[2]);
            }
            else
                strcpy(temp_path, ".");

            dp = opendir(temp_path);

            // Error handling for opendir
            if (dp == NULL) {
                if (errno == ENOTDIR) {
                    if (lstat(temp_path, &buf) == 0) {
                        print_file_info(&buf, argv[2], result_buff);
                    }
                    return 0;
                }
                else if (errno == EACCES) 
                    strcat(result_buff, "Error: Permission denied\n\n");
                else 
                    strcpy(result_buff, "Error: no such file exist\n\n");
                
                return -1;
            }
            else {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (argc == 3) {
                        if (ab_flag) 
                            snprintf(path, sizeof(path), "%s/%s", argv[2], dirp->d_name);
                        else
                            snprintf(path, sizeof(path), "./%s/%s", argv[2], dirp->d_name);
                    }
                    else
                        snprintf(path, sizeof(path), "./%s", dirp->d_name);

                    // Get file status
                    if (lstat(path, &buf) == 0) {
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
                char path[1024];
                for (int i = 0; i < index; i++) {
                    if (argc == 3) {
                        if (ab_flag)
                            snprintf(path, sizeof(path), "%s/%s", argv[2], temp[i]);
                        else
                            snprintf(path, sizeof(path), "./%s/%s", argv[2], temp[i]);
                    }
                    else
                        snprintf(path, sizeof(path), "./%s", temp[i]);

                    if (lstat(path, &buf) == 0) {
                        print_file_info(&buf, temp[i], result_buff);
                    }
                }
            }
        }
        // given path without option
        else {
            char temp_path[1024];
            int ab_flag = 0;
            // open directory with path
            if (argv[1][0]== '/') {
                strcpy(temp_path, argv[1]);
                ab_flag = 1;
            }
            else
                snprintf(temp_path, sizeof(temp_path), "./%s", argv[1]);

            dp = opendir(temp_path);

            // Error handling for opendir
            if (dp == NULL) {
                if (errno == ENOTDIR) {
                    strcat(result_buff, argv[1]);
                    strcat(result_buff, "\n");
                    return 0;
                }
                else if (errno == EACCES) 
                    strcat(result_buff, "Error: Permission denied\n\n");
                else 
                    strcpy(result_buff, "Error: no such file exist\n\n");
                
                return -1;
            }
            else {
                while ((dirp = readdir(dp)) != NULL) {                
                    // Build the full path to the file
                    char path[1024];
                    if (ab_flag)
                        snprintf(path, sizeof(path), "%s/%s", argv[1], dirp->d_name);
                    else
                        snprintf(path, sizeof(path), "./%s/%s", argv[1], dirp->d_name);
                    
                    // Get file status
                    if (lstat(path, &buf) == 0) {
                        if (dirp->d_name[0] != '.') {
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

                if(index != 0) {
                    // sort buffer strings before print
                    qsort(temp, index, sizeof(char *), compare_strings);
                    for (int i = 0; i < index; i++) {
                        strcat(result_buff, temp[i]);
                        strcat(result_buff, "\n");
                    }
                }
                else
                    strcpy(result_buff, "\n");
            }
        }

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

//////////////////////////////////////////////////////////////////////
// compare_strings                                                  //
// =================================================================//
// Input: a -> pointer to the first string's pointer                //
//        b -> pointer to the second string's pointer               //
//                                                                  //
// Output: int - Negative if 'a' is less than 'b',                  //
//               zero if 'a' and 'b' are equal,                    //
//               positive if 'a' is greater than 'b'.              //
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

    sprintf(perms , "%c%c%c%c%c%c%c%c%c%c", 't',
            buf->st_mode & S_IRUSR ? 'r' : '-',
            buf->st_mode & S_IWUSR ? 'w' : '-',
            buf->st_mode & S_IXUSR ? 'x' : '-',
            buf->st_mode & S_IRGRP ? 'r' : '-',
            buf->st_mode & S_IWGRP ? 'w' : '-',
            buf->st_mode & S_IXGRP ? 'x' : '-',
            buf->st_mode & S_IROTH ? 'r' : '-',
            buf->st_mode & S_IWOTH ? 'w' : '-',
            buf->st_mode & S_IXOTH ? 'x' : '-');

    // Determine the file type and set the first character of the permissions string
    if (S_ISDIR(buf->st_mode))
        perms[0] = 'd'; // Directory
    else if (S_ISREG(buf->st_mode))
        perms[0] = '-'; // Regular file
    else if (S_ISLNK(buf->st_mode))
        perms[0] = 'l'; // Symbolic link
    else if (S_ISBLK(buf->st_mode))
        perms[0] = 'b'; // Block device
    else if (S_ISCHR(buf->st_mode))
        perms[0] = 'c'; // Character device
    else if (S_ISFIFO(buf->st_mode))
        perms[0] = 'p'; // FIFO or pipe
    else if (S_ISSOCK(buf->st_mode))
        perms[0] = 's'; // Socket
    else
        perms[0] = '?'; // Unknown file type    

    char temp1[1024];

    struct passwd *pwd = getpwuid(buf->st_uid);
    struct group *grp = getgrgid(buf->st_gid);
    char date[20];
    strftime(date, sizeof(date), "%b %d %H:%M", localtime(&buf->st_mtime));

    sprintf(temp1, "%s %3ld %s %s %6ld %s %s\n", 
            perms, buf->st_nlink, pwd->pw_name, grp->gr_name, buf->st_size, date, name);
    strcat(output, temp1);
}