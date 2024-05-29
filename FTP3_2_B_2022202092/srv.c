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

#define MAX_BUF 3024
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
    const char *success_msg = "200 Port command successful\n";
    const char *data_msg = "150 Opening data connection for directory list\n";
    const char *transmission_msg = "226 Result is sent successfully.\n";

    char *host_ip;
    unsigned int port_num;

    // control connection
    char control_buff[MAX_BUF], data_buff[MAX_BUF], result_buff[MAX_BUF];
    int port = atoi(argv[1]);
    struct sockaddr_in clientaddr;
    int sockfd_control, clientfd_control, n;
 
    /////////////////////// address and PORT for control connect ////////////////////////////
    memset(&clientaddr, 0, sizeof(clientaddr)); // initialize server
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_addr.s_addr = INADDR_ANY;
    clientaddr.sin_port = htons((uint16_t)port);
    /////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////// make control connection //////////////////////////////////
    sockfd_control = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_control < 0) {
        perror("server control socket creation failed");  // Print error if socket creation fails
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd_control, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
        perror("bind");
        close(sockfd_control); 
        exit(1);
    }
    if (listen(sockfd_control, 5)) {
        perror("listen");
        exit(1);
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    if(sockfd_control != -1) {
        socklen_t client_len = sizeof(clientaddr);
        clientfd_control = accept(sockfd_control, (struct sockaddr *)&clientaddr, &client_len);

        while (1) {
            read(clientfd_control, control_buff, MAX_BUF); // receive converted cmd
            printf("%s\n", control_buff);
            host_ip = convert_str_to_addr(control_buff, &port_num);
            memset(control_buff, 0, MAX_BUF);

            struct sockaddr_in servaddr;
            int sockfd_data;
            ///////////////////////// address and PORT for data connect /////////////////////////////
            memset(&servaddr, 0, sizeof(servaddr));  // Clear structure
            servaddr.sin_family = AF_INET;  // Set family to IPv4
            servaddr.sin_addr.s_addr = inet_addr(host_ip);  // Set IP address
            servaddr.sin_port = htons((uint16_t)port_num);  // Set port number
            /////////////////////////////////////////////////////////////////////////////////////////

            ////////////////////////////// make control connection //////////////////////////////////
            sockfd_data = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket for IPv4 and TCP
            if (sockfd_data < 0) {
                perror("server data socket creation failed");  // Print error if socket creation fails
                exit(EXIT_FAILURE);
            }

            if (connect(sockfd_data, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("server data connection failed");
                exit(EXIT_FAILURE);
            }
            
            // send and print 200 control connection success
            n = write(STDOUT_FILENO, success_msg, strlen(success_msg));
            if (n < 0) {
                perror("Error writing to stdout");
                exit(EXIT_FAILURE);
            }
            n = write(clientfd_control, success_msg, strlen(success_msg));
            if (n < 0) {
                perror("Error writing to control socket");
                exit(EXIT_FAILURE);
            }

            // receive FTP cmd and print
            n = read(sockfd_data, data_buff, MAX_BUF);
            if (n < 0) {
                perror("Error reading from data socket");
                exit(EXIT_FAILURE);
            }
            write(STDOUT_FILENO, data_buff, n);
            printf("\n");

            // send and print 150 control connection success
            n = write(STDOUT_FILENO, data_msg, strlen(data_msg));
            n = write(clientfd_control, data_msg, strlen(data_msg));
            if (n < 0) {
                perror("Error writing to control socket");
                exit(EXIT_FAILURE);
            }

            memset(result_buff, 0, MAX_BUF);
            cmd_process(data_buff, result_buff);
            // send FTP result
            n = write(sockfd_data, result_buff, strlen(result_buff));
            if (n < 0) {
                perror("Error reading from data socket");
                exit(EXIT_FAILURE);
            }
            sleep(1);

            // send and print 226 transmission success
            n = write(STDOUT_FILENO, transmission_msg, strlen(transmission_msg));
            if (n < 0) {
                perror("Error writing to stdout");
                exit(EXIT_FAILURE);
            }
            n = write(clientfd_control, transmission_msg, strlen(transmission_msg));
            if (n < 0) {
                perror("Error writing to control socket");
                exit(EXIT_FAILURE);
            }

            break;
        }
        close(sockfd_control);
    }

    return 0;
}

//client로부터 받은 PORT명령어에 붙은 IP주소와 포트번호를 변경
char* convert_str_to_addr(char *str, unsigned int *port) { 
    char temp[10];
    unsigned int ip_parts[4], port_parts[2];
    char *addr = malloc(INET_ADDRSTRLEN);
    if (addr == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    sscanf(str, "%s %u,%u,%u,%u,%u,%u", temp, &ip_parts[0], &ip_parts[1], &ip_parts[2], &ip_parts[3], &port_parts[0], &port_parts[1]);

    snprintf(addr, INET_ADDRSTRLEN, "%u.%u.%u.%u", ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3]);

    *port = (port_parts[1] << 8) | port_parts[0];

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