//////////////////////////////////////////////////////////////////////
// File Name : cli.c                                                //
// Date : 2024/04/15                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #1-3 ( ftp server )        //
// Description : The program work as Linux terminal commands ls, dir//
//              pwd, cd, mkdir, delete, rmdir, rename, quit command.//
//              This file work for client command parsing and       //
//              change to FTP command and pass it to server file    //
//////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: agrc -> Argument count,                                   //
//        argv** -> Array of command line argument strings          //
//                                                                  //
// Output: int - Program exit status 0 for normal termination       //
//                                                                  //
// Purpose: Parsing command-line arguments using getopt             //
//          and displaying non-option arguments                     //
//////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    char *buffer = (char*)malloc(1024 * sizeof(char)); // Buffer to store command outputs or messages

    char c;// Variable to store the option character returned by getopt
    int aflag = 0, lflag = 0; // flag for option management
    int oflag = 0, xflag = 0, yflag = 0, qflag = 0, rflag = 0; // flag for error management
    opterr = 0; // Disable automatic error reporting by getopt

    // If first argument is "ls"
    if (strcmp(argv[1], "ls") == 0) {
        // option check for ls
        while ((c = getopt(argc, argv, "al")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case 'a': // If option 'a' is given
                    aflag = 1; // Set 'aflag' as 1
                    break;
                case 'l': // If option 'b' is given
                    lflag = 1; // Set 'bflag' as 1
                    break;
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        strcpy(buffer, "NLST"); // Start building the NLST command

        // Append appropriate flags to the command
        if (aflag == 1 && lflag == 0) 
            strcat(buffer, " -a");        
        else if (aflag == 0 && lflag == 1) 
            strcat(buffer, " -l");
        else if (aflag == 1 && lflag == 1) 
            strcat(buffer, " -al");
        
        // Append path if specified
        if (strcmp(argv[argc - 1], "ls") != 0) {
            if (strcmp(argv[argc - 2], "ls") == 0) {
                strcat(buffer, " ");
                strcat(buffer, argv[argc - 1]); // store path to temp
            }
            else
                xflag = 1; // set error flag x: too many argument
        }
    }
    // 'dir' command
    else if (strcmp(argv[1], "dir") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }
        
        // 'dir' command does not take any options, only a path
        strcpy(buffer, "LIST");

        if (strcmp(argv[argc - 1], "dir") != 0) {
            if (strcmp(argv[argc - 2], "dir") == 0) {
                strcat(buffer, " ");
                strcat(buffer, argv[argc - 1]); // store path to buffer
            }
            else
                qflag = 1;
        }
    }
    // 'pwd' command
    else if (strcmp(argv[1], "pwd") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }
        
        // 'pwd' command does not take any option and path
        strcpy(buffer, "PWD");

        if (strcmp(argv[argc - 1], "pwd") != 0) {
            qflag = 1;
        }
    }
    // 'cd' command
    else if (strcmp(argv[1], "cd") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        // append srv command to buffer
        strcpy(buffer, "CWD");
        if (strcmp(argv[argc - 1], "cd") != 0) {
            if (strcmp(argv[argc - 2], "cd") == 0) {
                if (strcmp(argv[argc - 1], "..") == 0)
                    strcpy(buffer, "CDUP");
                else {
                    strcat(buffer, " ");
                    strcat(buffer, argv[argc - 1]); // store path to buffer
                }
            }
            else
                xflag = 1;
        }
        else
            yflag = 1;
    } 
    // 'mkdir' command
    else if (strcmp(argv[1], "mkdir") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        // append srv command to buffer
        strcpy(buffer, "MKD");
        if (strcmp(argv[argc - 1], "mkdir") != 0) {
            for (int i = 2; i < argc; i++) {
                strcat(buffer, " ");
                strcat(buffer, argv[i]);
            }
        }
        else {
            yflag = 1;
        }
    }
    // 'delete', 'rmdir' command
    else if (strcmp(argv[1], "delete") == 0 || strcmp(argv[1], "rmdir") == 0) {
        (strcmp(argv[1], "delete") == 0) ? strcpy(buffer, "DELE") : strcpy(buffer, "RMD");

        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        // append srv command to buffer
        if (argc == 2)
            yflag = 1;
        else {
            for (int i = 2; i < argc; i++) {
                strcat(buffer, " ");
                strcat(buffer, argv[i]);
            }
        }
    }
    // 'rename' command
    else if (strcmp(argv[1], "rename") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        // append srv command to buffer
        strcpy(buffer, "RNFR ");
        if (argc != 4)
            rflag = 1;
        else {
            strcat(buffer, argv[2]);
            strcat(buffer, " RNTO ");
            strcat(buffer, argv[3]);
        }

    }
    // 'quit' command
    else if (strcmp(argv[1], "quit") == 0) {
        // option check for dir
        while ((c = getopt(argc, argv, "")) != -1) {
            // Switch statement to handle the option character returned by getopt()
            switch (c) {
                case '?': // If an unrecognized option is encountered
                    oflag = 1;
            }
        }

        // 'quit' command does not take any options or path
        strcpy(buffer, "QUIT");
        if (argc != 2)
            qflag = 1;
    }
    // Unknown command
    else {
        strcpy(buffer, "Error: !");
        write(STDOUT_FILENO, buffer, 1024);
        return 1;
    }

    // process error flag
    if (rflag)
        strcpy(buffer, "Error: r");
    else if (qflag) 
        oflag ? strcpy(buffer, "Error: o") : strcpy(buffer, "Error: q");
    else if (oflag)
        strcpy(buffer, "Error: o");
    else if (xflag)
        strcpy(buffer, "Error: x");
    else if (yflag)
        strcpy(buffer, "Error: y");

    //printf("%s", buffer);
    write(STDOUT_FILENO, buffer, 1024);
    free(buffer);

    return 0;
}

