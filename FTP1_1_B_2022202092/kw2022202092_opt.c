//////////////////////////////////////////////////////////////////////
// File Name : kw2022202092_opt.c                                   //
// Date : 2024/03/31                                                //
// OS : Ubuntu 20.04.6 LTS 64bits                                   //
//                                                                  //
// Author : Sunwoo Yeon                                             //
// Student ID : 2022202092                                          //
// -----------------------------------------------------------------//
// Title : System Programming Assignment #1-1 ( ftp server )        //
// Description : The program get user input with getopt() and parse //
//               each parameters a, b, and c                        //
//////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>

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

int main (int argc, char **argv) {
    int aflag = 0, bflag = 0;  // Flag variables to track whether options have been set
    char *cvalue = NULL; // Pointer for the argument 'c' option
    int index, c; // Variables for the loop index and the character for the current option
    opterr = 0; // External variable in getopt.h, set to zero for non-error

    // Loop to process each command-line argument
    while ((c = getopt(argc, argv, "abc:")) != -1) {
        // Switch statement to handle the option character returned by getopt()
        switch (c) {
            case 'a': // If option 'a' is given
                aflag++; // Set 'aflag' as 1
                break;
            case 'b': // If option 'b' is given
                bflag++; // Set 'bflag' as 1
                break;
            case 'c': // If option 'c' is given, expect a string as its argument
                cvalue = optarg; // Set cvalue as optarg
                break;
            case '?': // If an unrecognized option is encountered
                opterr = 1; // Set opterr 1
        }
    }

    // Print the status of the command-line options
    printf("aflag = %d, bflag = %d, cvalue = %s\n", aflag, bflag, cvalue);
    
    // Process for remaining non-option arguments
    for (index = optind; index < argc; index++) {
        printf("Non-option argument %s\n", argv[index]);
    }

    // Return 0 to indicate successful execution
    return 0;
}