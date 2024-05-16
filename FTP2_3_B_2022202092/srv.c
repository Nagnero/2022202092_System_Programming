//////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                //
// Date : 2024/05/12                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #2-3 ( ftp server )        //
// Description : The program work as Linux terminal commands ls, dir//
//              pwd, cd, mkdir, delete, rmdir, rename, quit command.//
//              This file work for receiving FTP commands from      //
//              client and execute appropriate work                 //
//////////////////////////////////////////////////////////////////////
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
#include <netinet/in.h> 
#include <sys/wait.h> 
#include <signal.h>

#define BUF_SIZE 100000

void sh_chld(int); // signal handler for SIGCHLD 
void sh_alrm(int); // signal handler for SIGALRM
void sh_int(int signo); // signal handler for SIGINT
int cmd_process(char* buff, char* result_buff);
int compare_strings(const void *a, const void *b);
void print_file_info(struct stat *buf, char *name, char* output);

// Define the pid_port structure
struct pid_port {
    pid_t pid;
    int port;
    int client_fd; 
    time_t start_time;
};

// Define the node structure for the linked list
struct node {
    struct pid_port data;
    struct node* next;
};

// Function to create a new node
struct node* create_node(pid_t pid, int port, int client_fd) {
    struct node* new_node = (struct node*)malloc(sizeof(struct node));
    if (!new_node) {
        perror("Failed to allocate memory for new node");
        exit(1);
    }

    new_node->data.pid = pid;
    new_node->data.port = port;
    new_node->data.client_fd = client_fd; // client_fd 저장
    new_node->data.start_time = time(NULL);
    new_node->next = NULL;

    return new_node;
}

// Function to add a node to the end of the list
void add_node(struct node** head, pid_t pid, int port, int client_fd) {
    struct node* newNode = create_node(pid, port, client_fd);
    if (*head == NULL) {
        // If the list is empty, make the new node the head
        *head = newNode;
    } else {
        // Otherwise, find the last node and append the new node
        struct node* curNode = *head;
        while (curNode->next != NULL) {
            curNode = curNode->next;
        }
        curNode->next = newNode;
    }
}

void remove_node(struct node **head, pid_t pid) {
    struct node *temp = *head, *prev = NULL;
    while (temp != NULL && temp->data.pid != pid) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return;
    if (prev == NULL) {
        *head = temp->next;
    } else {
        prev->next = temp->next;
    }
    free(temp);
}

struct node* head = NULL;
int child_cnt;
int server_fd, client_fd;
int exiting = 0;

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: argc -> Number of command line arguments                  //
//        argv -> Array of command line argument strings            //
//                                                                  //
// Output: int - Returns 0 for normal termination                   //
//               Returns 1 for error termination                    //
//                                                                  //
// Purpose: Set up server to accept client connections, fork child  //
//          processes to handle client requests, and manage signal  //
//          handling for process control.                           //
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
    child_cnt = 0;
    char buff[BUF_SIZE], result_buff[BUF_SIZE];
    int n;
    struct sockaddr_in server_addr, client_addr; 
    socklen_t len;

    // Set up signal handlers
    if (signal(SIGCHLD, sh_chld) == SIG_ERR) {
        perror("Cannot handle SIGCHLD");
        exit(1);
    }
    if (signal(SIGALRM, sh_alrm) == SIG_ERR) {
        perror("Cannot handle SIGALRM");
        exit(1);
    }
    if (signal(SIGINT, sh_int) == SIG_ERR) {
        perror("Cannot handle SIGINT");
        exit(1);
    }

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket create fail");
        exit(1);
    }

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
            close(server_fd);
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

                char output[128];
                strcpy(output, buff);
                //char temp[128];

                snprintf(output, sizeof(output), "%-40.40s\t[%d]\n", buff, getpid());
                write(STDERR_FILENO, output, sizeof(output));
                
                if (cmd_process(buff, result_buff) < 0) {
                    write(STDERR_FILENO, "Error: No such file or directory\n", sizeof("Error: No such file or directory\n"));
                }

                if (strcmp(result_buff, "QUIT\n") == 0) {
                    write(client_fd, result_buff, strlen(result_buff));
                    printf("Client(%d)'s Release\n\n", getpid());
                    break;
                }
                else if (strcmp(result_buff, "Error: No such file or directory\n") == 0) {
                    write(client_fd, result_buff, strlen(result_buff));
                    write(STDOUT_FILENO, result_buff, strlen(result_buff));
                    memset(buff, 0, sizeof(buff));
                    memset(result_buff, 0, sizeof(result_buff));
                }
                else {
                    write(client_fd, result_buff, strlen(result_buff));
                    memset(buff, 0, sizeof(buff));
                    memset(result_buff, 0, sizeof(result_buff));
                }
            }

            close(client_fd);
            exit(0);
        }
        else { // parent
            add_node(&head, pid, ntohs(client_addr.sin_port), client_fd);
            char parent_output[BUF_SIZE] = {};
            /* display client ip and port */
            char temp_output[1000];
            snprintf(temp_output, sizeof(temp_output), 
                    "==========Client info==========\n"
                    "client IP: %s\n\n"
                    "client port: %d\n"
                    "===============================\n",
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            write(STDOUT_FILENO, temp_output, strlen(temp_output));
            sprintf(parent_output,  "Child Process ID : %d\n", pid);
            write(STDOUT_FILENO, parent_output, strlen(parent_output));
            memset(temp_output, 0, sizeof(temp_output));
            alarm(1);
        }

        child_cnt++;
        close(client_fd);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
// sh_chld                                                          //
// =================================================================//
// Input: signum -> Signal number (SIGCHLD)                         //
//                                                                  //
// Output: None                                                     //
//                                                                  //
// Purpose: Signal handler for SIGCHLD. Handles termination of child//
//          processes, updates child process count, and removes     //
//          terminated processes from the linked list. It also      //
//          prints the current state of active child processes.     //
//////////////////////////////////////////////////////////////////////
void sh_chld(int signum) {
    usleep(600); // delay for server closing
    if (exiting) return; // ignore process terminated by SIGTERM or during exiting
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        child_cnt--;
        remove_node(&head, pid);
    }
    char cur_state[BUF_SIZE];
    snprintf(cur_state, sizeof(cur_state), "Current Number of Clients: %d\n", child_cnt);

    if (child_cnt > 0) {
        strcat(cur_state, "     PID    PORT    TIME\n");

        struct node* curNode = head;
        while (curNode) {
            time_t current_time = time(NULL);
            int elapsed_time = difftime(current_time, curNode->data.start_time);  // Calculate elapsed time
            char t[128];
            snprintf(t, sizeof(t), "%8d%8d%8d\n", curNode->data.pid, curNode->data.port, elapsed_time);
            strncat(cur_state, t, BUF_SIZE - strlen(cur_state) - 1);
            curNode = curNode->next;
        }
        strcat(cur_state, "\n");
    }

    write(STDOUT_FILENO, cur_state, strlen(cur_state));
}

//////////////////////////////////////////////////////////////////////
// sh_alrm                                                          //
// =================================================================//
// Input: signum -> Signal number (SIGALRM)                         //
//                                                                  //
// Output: None                                                     //
//                                                                  //
// Purpose: Signal handler for SIGALRM. Sets the alarm for periodic //
//          execution, and prints the current state of active child //
//          processes.                                              //
//////////////////////////////////////////////////////////////////////
void sh_alrm(int signum) {
    //alarm(10);
    if (child_cnt != 0) {
        char cur_state[BUF_SIZE];
        snprintf(cur_state, sizeof(cur_state), "Current Number of Clients: %d\n", child_cnt);

        if (child_cnt > 0) {
            strcat(cur_state, "     PID    PORT    TIME\n");

            struct node* curNode = head;
            while (curNode) {
                time_t current_time = time(NULL);
                int elapsed_time = difftime(current_time, curNode->data.start_time);  // Calculate elapsed time
                char t[128];
                snprintf(t, sizeof(t), "%8d%8d%8d\n", curNode->data.pid, curNode->data.port, elapsed_time);
                strncat(cur_state, t, BUF_SIZE - strlen(cur_state) - 1);
                curNode = curNode->next;
            }
            strcat(cur_state, "\n");
        }

        write(STDOUT_FILENO, cur_state, strlen(cur_state));
    }
}

//////////////////////////////////////////////////////////////////////
// sh_int                                                           //
// =================================================================//
// Input: signo -> Signal number (SIGINT)                           //
//                                                                  //
// Output: None                                                     //
//                                                                  //
// Purpose: Signal handler for SIGINT. Terminates all child         //
//          processes, waits for them to exit, and exits the program//
//          gracefully.                                             //
//////////////////////////////////////////////////////////////////////
void sh_int(int signo) {
    // Terminate all child processes
    struct node* current = head;
    exiting = 1; // Set exiting flag to prevent sh_chld from processing

    while (current != NULL) {
        close(current->data.client_fd);
        if (kill(current->data.pid, SIGTERM) == -1) {
            perror("kill error");
        }
        if (kill(current->data.pid - 1, SIGTERM) == -1) {
            perror("client kill error");
        }
        current = current->next;
    }

    // Wait for all child processes to exit
    int status;
    while (waitpid(-1, &status, 0) > 0);

    // Exit the program
    exit(0);
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
int cmd_process(char* buff, char* result_buff) {
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
        else if (argv[1][0] == 'y')
            strcpy(result_buff, "Error: argument is required\n\n");
        
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
            
            if (index != 0) {
                // sort buffer strings before print
                qsort(temp, index, sizeof(char *), compare_strings);
                
                for (int i = 0; i < index; i++) {
                    // Print each string with a fixed width of 25 characters
                    strcat(result_buff, temp[i]);
                    strcat(result_buff, " \t");

                    // After printing 4 names, print a newline
                    if ((i + 1) % 4 == 0) 
                        strcat(result_buff, "\n");                    
                }

                // Add a final newline if the last line didn't end with one
                if (index % 4 != 0) 
                    strcat(result_buff, "\n");
            }
            else
                strcpy(result_buff, "\n");
        }
        else if (strcmp(argv[1], "-a") == 0) {
            char temp_path[1024];
            int ab_flag = 0; // check absolute path
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
                    strcpy(result_buff, "Error: No such file or directory\n\n");
                
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
                    // Print each string with a fixed width of 25 characters
                    strcat(result_buff, temp[i]);
                    strcat(result_buff, " \t");

                    // After printing 4 names, print a newline
                    if ((i + 1) % 4 == 0) 
                        strcat(result_buff, "\n");                    
                }

                // Add a final newline if the last line didn't end with one
                if (index % 4 != 0) 
                    strcat(result_buff, "\n");
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
                    strcpy(result_buff, "Error: No such file or directory\n\n");
                
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
                    strcpy(result_buff, "Error: No such file or directory\n\n");
                
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
                    strcpy(result_buff, "Error: No such file or directory\n\n");
                
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
    else if (strcmp(argv[0], "LIST") == 0) {        
        DIR *dp; // Directory stream
        struct dirent *dirp; // Pointer for directory entry
        struct stat buf;
        char* temp[10000];
        char temp_path[1024];
        int index = 0;
        int ab_flag = 0; // check absolute path

        // open directory with path
        if (argc == 2) {
            if (argv[1][0] == '/') {
                strcpy(temp_path, argv[1]);
                ab_flag = 1;
            }
            else
                snprintf(temp_path, sizeof(temp_path), "./%s", argv[1]);
        }
        else 
            strcpy(temp_path, ".");

        dp = opendir(temp_path);

        // Error handling for opendir
        if (dp == NULL) {
            // Specific error handling
            if (errno == ENOTDIR) { // check if it is file
                    if (lstat(temp_path, &buf) == 0) {
                        print_file_info(&buf, argv[1], result_buff);
                    }
                    return 0;
                }
                else if (errno == EACCES) 
                    strcat(result_buff, "Error: Permission denied\n\n");
                else 
                    strcpy(result_buff, "Error: No such file or directory\n\n");
                
                return -1;
        }
        else {
            while ((dirp = readdir(dp)) != NULL) {                
                // Build the full path to the file
                char path[1024];
                if (argc == 2) {
                    if (ab_flag)
                        snprintf(path, sizeof(path), "%s/%s", argv[1], dirp->d_name);
                    else
                        snprintf(path, sizeof(path), "./%s/%s", argv[1], dirp->d_name);
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
            char path[1024];
            
            for (int i = 0; i < index; i++) {
                if (argc == 2) {
                    if (ab_flag)
                        snprintf(path, sizeof(path), "%s/%s", argv[1], temp[i]);
                    else
                        snprintf(path, sizeof(path), "./%s/%s", argv[1], temp[i]);
                }
                else
                    snprintf(path, sizeof(path), "./%s", temp[i]);

                if (lstat(path, &buf) == 0) {
                    print_file_info(&buf, temp[i], result_buff);
                }
            }
        }

        return 0;
    }
    // 'PWD' command
    else if (strcmp(argv[0], "PWD") == 0) {
        char path[1024];
        getcwd(path, 1024);
        strcat(result_buff, "\"");
        strcat(result_buff, path);
        strcat(result_buff, "\" is current directory\n");

        return 0;
    }
    // 'CWD' command
    else if (strcmp(argv[0], "CWD") == 0) {
        char path[1024];
        char normalized_path[1024] = {0};

        getcwd(path, sizeof(path));

        // Absolute path
        if (argv[1][0] == '/') {
            strcpy(path, argv[1]);
        } else {
            strcat(path, "/");
            strcat(path, argv[1]);
        }

        // Normalize the path
        char *components[1024];
        int comp_count = 0;
        
        char temp_path[1024];
        strcpy(temp_path, path);
        
        // Split the path into components
        token = strtok(temp_path, "/");
        while (token != NULL) {
            if (strcmp(token, ".") == 0) {
                // Ignore '.'
            } else if (strcmp(token, "..") == 0) {
                // Handle '..' by removing the last component
                if (comp_count > 0) {
                    comp_count--;
                }
            } else {
                components[comp_count++] = token;
            }
            token = strtok(NULL, "/");
        }
        
        // Rebuild the normalized path
        normalized_path[0] = '\0';
        for (int i = 0; i < comp_count; i++) {
            strcat(normalized_path, "/");
            strcat(normalized_path, components[i]);
        }

        // Handle root directory case
        if (strlen(normalized_path) == 0) {
            strcpy(normalized_path, "/");
        }


        // Check if directory exists
        struct stat statbuf;
        if (stat(normalized_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
            strcpy(result_buff, "Error: No such file or directory\n");
        } else {
            // Change directory
            if (chdir(normalized_path) == -1) {
                strcpy(result_buff, "Error: failed to change directory\n");
            } else {
                getcwd(path, sizeof(path));
                strcat(result_buff, "\"");
                strcat(result_buff, path);
                strcat(result_buff, "\" is current directory\n");
            }
        }

        return 0;
    }
    // 'CDUP' command
    else if (strcmp(argv[0], "CDUP") == 0) {
        char path[1024];
        char parent_path[1024];
        getcwd(path, 1024);
        int cnt = 0;
        char* str[64];
        
        token = strtok(path, "/");
        while (token != NULL) {
            str[cnt++] = token; // Assign the token to argv and increment argc
            token = strtok(NULL, "/"); // Continue tokenizing the string
        }
        str[cnt] = NULL;

        strcpy(parent_path, "/");
        strcat(parent_path, str[0]);
        for (int i = 1; i < cnt - 1; i++) {
            strcat(parent_path, "/");
            strcat(parent_path, str[i]);
        }

        // Check if directory exists
        struct stat statbuf;
        if (stat(parent_path, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
            strcpy(result_buff, "Error: No such file or directory\n");
        } else {
            // Change directory
            if (chdir(parent_path) == -1) {
                strcat(result_buff, "Error: failed to change directory\n");
            } else {
                strcat(result_buff, "\"");
                strcat(result_buff, parent_path);
                strcat(result_buff, "\" is current directory\n");
            }
        }

        return 0;
    }
    // 'MKD' command
    else if (strcmp(argv[0], "MKD") == 0) {
        strcpy(result_buff, "");
        for (int i = 1; i < argc; i++) {            
            if (mkdir(argv[i], 0777)) {
                strcat(result_buff, "Error: cannot create directory '");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "': File exists\n");
            }
            else {
                strcat(result_buff, "Make new directory ");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "\n");
            }
        }

        return 0;
    }
    // 'DELE' command
    else if (strcmp(argv[0], "DELE") == 0) {
        strcpy(result_buff, "");
        for (int i = 1; i < argc; i++) {
            if (unlink(argv[i])) {
                strcat(result_buff, "Error: failed to delete '");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "'\n");
            }
            else {
                strcat(result_buff, "Delete file ");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "\n");
            }
        }

        return 0;
    }
    // 'RMD' command
    else if (strcmp(argv[0], "RMD") == 0) {
        strcpy(result_buff, "");
        for (int i = 1; i < argc; i++) {
            if (rmdir(argv[i])) {
                strcat(result_buff, "Error: failed to remove '");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "'\n");
            }
            else {
                strcat(result_buff, "Delete directory ");
                strcat(result_buff, argv[i]);
                strcat(result_buff, "\n");
            }
        }

        return 0;
    }
    // 'RNFR' commands
    else if (strcmp(argv[0], "RNFR") == 0) {
        strcpy(result_buff, "");

        // RNFR don't exist
        if (access(argv[1], F_OK) != 0) {
            strcat(result_buff, "Error: target file doesn't exists\n");
        }
        // RNTO already exist
        else if (access(argv[3], F_OK) == 0) {
            strcat(result_buff, "Error: name to change already exists\n");
        }
        else {
            rename(argv[1], argv[3]);
            char output[1024];
            snprintf(output, sizeof(output), "%s %s\n%s %s\n", argv[0], argv[1], argv[2], argv[3]);
            strcat(result_buff, output);
        }
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