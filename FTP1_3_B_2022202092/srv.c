//////////////////////////////////////////////////////////////////////
// File Name : srv.c                                                //
// Date : 2024/04/15                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #1-3 ( ftp server )        //
// Description : The program work as Linux terminal commands ls, dir//
//              pwd, cd, mkdir, delete, rmdir, rename, quit command.//
//              This file work for receiving FTP commands from      //
//              client and execute appropriate work                 //
//////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

char* output;

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
void print_file_info(struct stat *buf, char *name) {
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

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: None                                                      //
//                                                                  //
// Output: int - Program exit status 0 for normal termination       //
//               1 on error termination                             //
//                                                                  //
// Purpose: read buffer that is passed from client and parse data   //
//         in buffer to execute different work                      //
//////////////////////////////////////////////////////////////////////
int main() {
    int argc = 0;
    char* argv[64];
    char* token;
    char path[1024];
    output = (char*)malloc(1024 * sizeof(char));
    char *buffer = (char*)malloc(1024 * sizeof(char));
    read(STDIN_FILENO, buffer, 1024);

    // parse data from client buffer
    token = strtok(buffer, " ");
    while (token != NULL) {
        argv[argc++] = token; // Assign the token to argv and increment argc
        token = strtok(NULL, " "); // Continue tokenizing the string
    }
    argv[argc] = NULL;

    // printing error state
    if (strcmp(argv[0], "Error:") == 0) {
        if (argv[1][0] == '!')
            strcpy(output, "Error: unknown command\n");
        else if (argv[1][0] == 'r')
            strcpy(output, "Error: two arguments are required\n");
        else if (argv[1][0] == 'q')
            strcpy(output, "Error: arguments is not required\n");
        else if (argv[1][0] == 'o')
            strcpy(output, "Error: invalid option\n");
        else if (argv[1][0] == 'x')
            strcpy(output, "Error: too many argument\n");
        else if (argv[1][0] == 'y')
            strcpy(output, "Error: argument is required\n");
    }
    else if (strcmp(argv[0], "NLST") == 0) {
        // argv[1]: option, argv[2]: path
        DIR *dp; // Directory stream
        struct dirent *dirp; // Pointer for directory entry
        struct stat buf;
        char* temp[64];
        int index = 0;

        strcat(output, argv[0]);

        if (argc == 1) {
            strcat(output, "\n");
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
                strcat(output, dirp->d_name); // Print each directories' name
            }

            // sort buffer strings before print
            qsort(temp, index, sizeof(char *), compare_strings);
            int offset = strlen(output);
            for (int i = 0; i < index; i++) {
                offset += snprintf(output + offset, sizeof(output) - offset, "%-25s", temp[i]);
                if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                    offset += snprintf(output + offset, sizeof(output) - offset, "\n");
                }
            }
        }
        else if (strcmp(argv[1], "-a") == 0) {
            strcat(output, " -a\n");

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
                        strcat(output, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(output, "Error: cannot access\n");
                        break;
                    default:
                        strcat(output, "Error: no such file or directory\n");
                }
                write(STDOUT_FILENO, output, 1024);
                return 1;
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
                int offset = strlen(output);
                for (int i = 0; i < index; i++) {
                    offset += snprintf(output + offset, sizeof(output) - offset, "%-25s", temp[i]);
                    if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                        offset += snprintf(output + offset, sizeof(output) - offset, "\n");
                    }
                }
            }
        }
        else if (strcmp(argv[1], "-l") == 0) {
            strcat(output, " -l\n");

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
                        strcat(output, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(output, "Error: cannot access\n");
                        break;
                    default:
                        strcat(output, "Error: no such file or directory\n");
                }
                write(STDOUT_FILENO, output, 1024);
                return 1;
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
                        print_file_info(&buf, temp[i]);
                    }
                }
            }
        }
        else if (strcmp(argv[1], "-al") == 0) {
            strcat(output, " -al\n");

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
                        strcat(output, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(output, "Error: cannot access\n");
                        break;
                    default:
                        strcat(output, "Error: no such file or directory\n");
                }
                write(STDOUT_FILENO, output, 1024);
                return 1;
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
                        print_file_info(&buf, temp[i]);
                    }
                }
            }
        }
        // given path without option
        else {
            strcat(output, "\n");

            // open directory with path
            dp = opendir(argv[1]);

            // Error handling for opendir
            if (dp == NULL) {
                // Specific error handling
                switch (errno) {
                    case ENOENT:
                        strcat(output, "Error: no such file or directory\n");
                        break;
                    case EACCES:
                        strcat(output, "Error: cannot access\n");
                        break;
                    default:
                        strcat(output, "Error: no such file or directory\n");
                }
                write(STDOUT_FILENO, output, 1024);
                return 1;
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
                int offset = strlen(output);
                for (int i = 0; i < index; i++) {
                    offset += snprintf(output + offset, sizeof(output) - offset, "%-25s", temp[i]);
                    if ((i + 1) % 4 == 0) { // After printing 5 names, print a newline
                        offset += snprintf(output + offset, sizeof(output) - offset, "\n");
                    }
                }
            }
        }

        strcat(output, "\n");
        if (dp != NULL)
            closedir(dp); // Close the directory stream
    }
    else if (strcmp(argv[0], "LIST") == 0) {
        strcat(output, argv[0]);
        DIR *dp; // Directory stream
        struct dirent *dirp; // Pointer for directory entry
        char* temp[64];
        int index = 0;
        struct stat buf;


        // open directory with path
        if (argc == 2) {
            dp = opendir(argv[1]);
            strcat(output, " ");
            strcat(output, argv[1]);
            strcat(output, "\n");
        }
        else {
            dp = opendir(".");
            strcat(output, "\n");
        }

        // Error handling for opendir
        if (dp == NULL) {
            // Specific error handling
            switch (errno) {
                case ENOENT:
                    strcat(output, "Error: no such file or directory\n");
                    break;
                case EACCES:
                    strcat(output, "Error: cannot access\n");
                    break;
                default:
                    strcat(output, "Error: no such file or directory\n");
            }
            write(STDOUT_FILENO, output, 1024);
            return 1;
        }

        if (dp != NULL) {
            while ((dirp = readdir(dp)) != NULL) {                
                // Build the full path to the file
                char path[1024];
                if (argc == 2)
                    snprintf(path, sizeof(path), "./%s/%s", argv[1], dirp->d_name);
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
                if (argc == 2)
                    snprintf(path, sizeof(path), "./%s/%s", argv[1], temp[i]);
                else
                    snprintf(path, sizeof(path), "./%s", temp[i]);
                if (stat(path, &buf) == 0) {
                    print_file_info(&buf, temp[i]);
                }
            }
        }
    }
    // 'PWD' command
    else if (strcmp(argv[0], "PWD") == 0) {
        getcwd(path, 1024);
        strcat(output, "\"");
        strcat(output, path);
        strcat(output, "\" is current directory\n");
    }
    // 'CWD' command
    else if (strcmp(argv[0], "CWD") == 0) {
        strcpy(output, argv[0]);
        strcat(output, " ");
        strcat(output, argv[1]);
        strcat(output, "\n");

        getcwd(path, 1024);
        strcat(path, "/");
        strcat(path, argv[1]);
        // if directory not found
        if (chdir(path) == -1) {
            strcpy(output, "Error: directory not found\n");
        }
        else {
            getcwd(path, 1024);
            strcat(output, "\"");
            strcat(output, path);
            strcat(output, "\" is current directory\n");
        }
    }
    // 'CDUP' command
    else if (strcmp(argv[0], "CDUP") == 0) {
        strcpy(output, argv[0]);
        strcat(output, "\n");

        getcwd(path, 1024);
        int cnt = 0;
        char* str[64];
        token = strtok(path, "/");
        while (token != NULL) {
            str[cnt++] = token; // Assign the token to argv and increment argc
            token = strtok(NULL, "/"); // Continue tokenizing the string
        }
        str[cnt] = NULL;

        strcat(output, "\"");
        strcat(output, str[0]);
        for (int i = 1; i < cnt - 1; i++) {
            strcat(output, "/");
            strcat(output, str[i]);
        }
        strcat(output, "\" is current directory\n");

    }
    // 'MKD' command
    else if (strcmp(argv[0], "MKD") == 0) {
        strcpy(output, "");
        for (int i = 1; i < argc; i++) {            
            if (mkdir(argv[i], 0777)) {
                strcat(output, "Error: cannot create directory '");
                strcat(output, argv[i]);
                strcat(output, "': File exists\n");
            }
            else {
                strcat(output, argv[0]);
                strcat(output, " ");
                strcat(output, argv[i]);
                strcat(output, "\n");
            }
        }
    }
    // 'DELE' command
    else if (strcmp(argv[0], "DELE") == 0) {
        strcpy(output, "");
        for (int i = 1; i < argc; i++) {
            strcat(path, argv[i]);
            if (unlink(argv[i])) {
                strcat(output, "Error: failed to delete '");
                strcat(output, argv[i]);
                strcat(output, "'\n");
            }
            else {
                strcat(output, "DELE ");
                strcat(output, argv[i]);
                strcat(output, "\n");
            }
        }
    }
    // 'RMD' command
    else if (strcmp(argv[0], "RMD") == 0) {
        strcpy(output, "");
        for (int i = 1; i < argc; i++) {
            strcat(path, argv[i]);
            if (rmdir(argv[i])) {
                strcat(output, "Error: failed to remove '");
                strcat(output, argv[i]);
                strcat(output, "'\n");
            }
            else {
                strcat(output, "RMD ");
                strcat(output, argv[i]);
                strcat(output, "\n");
            }
        }
    }
    // 'RNFR' commands
    else if (strcmp(argv[0], "RNFR") == 0) {
        strcpy(output, "");

        // RNFR don't exist
        if (access(argv[1], F_OK) != 0) {
            strcat(output, "Error: target file doesn't exists\n");
        }
        // RNTO already exist
        else if (access(argv[3], F_OK) == 0) {
            strcat(output, "Error: name to change already exists\n");
        }
        else {
            rename(argv[1], argv[3]);
            strcat(output, argv[0]);
            strcat(output, " ");
            strcat(output, argv[1]);
            strcat(output, "\n");
            strcat(output, argv[2]);
            strcat(output, " ");
            strcat(output, argv[3]);
            strcat(output, "\n");
        }
    }
    // 'QUIT' command
    else if (strcmp(argv[0], "QUIT") == 0) {
        strcpy(output, "QUIT success\n");
        write(STDOUT_FILENO, output, 1024);
        exit(0);
    }

    write(STDOUT_FILENO, output, 1024);
    free(buffer);
    free(output);
    return 0;
}