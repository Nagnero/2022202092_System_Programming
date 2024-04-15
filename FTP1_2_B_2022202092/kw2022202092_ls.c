//////////////////////////////////////////////////////////////////////
// File Name : kw2022202092_opt.c                                   //
// Date : 2024/04/08                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #1-2 ( ftp server )        //
// Description : The program work similar as Linux 'ls' command =.  //
//               when it executes, it prints directories name       //
//////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////
// main                                                             //
// =================================================================//
// Input: agrc -> Argument count,                                   //
//        argv** -> Array of command line argument strings          //
//                                                                  //
// Output: int - Program exit status 0 for normal termination       //
//                                                                  //
// Purpose: get file path as argument and print each directories    //
//          name that exist at that path                            //
//////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    DIR *dp; // Directory stream
    struct dirent *dirp; // Pointer for directory entry

    if (argc > 2) {
        printf("only one directory path can be processed\n");
        return 0;
    }

    // Open the directory by given path
    dp = (argc == 1) ? opendir(".") : opendir(argv[1]);

    // Error handling for opendir
    if (dp == NULL) {
        // If failed, print an error message based on the type of error
        switch (errno) {
            case ENOENT: // Error code for no such directory
                printf("testls: cannot access '%s': No such directory\n", argv[1]);
                break;
            case EACCES: // Error code for access denied
                printf("testls: cannot access '%s': Access denied\n", argv[1]);
                break;
            default: // Other cases, generalize as no such directory
                printf("testls: cannot access '%s': No such directory\n", argv[1]);
        }
        return 1; // Exit with error status
    }

    // Read directories in a loop
    while ((dirp = readdir(dp)) != NULL) 
        printf("%s\n", dirp->d_name); // Print each directories' name
    
    closedir(dp); // Close the directory stream

    return 0;
}