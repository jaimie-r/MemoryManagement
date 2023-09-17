/**************************************************************************
 * C S 429 MM-lab
 * 
 * err_handler.h - This file contains the enums used for error handling, as
 * well as a couple of function definitions.
 * 
 * Copyright (c) 2021. S. Chatterjee, X. Shen, T. Byrd, M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/ 

/* This enum represents the various levels used by the logging system. */
typedef enum {
    LOG_INFO,       // print a message to the console
    LOG_WARNING,    // print a warning message and ignore the current input
    LOG_ERROR,      // print an error message and ignore the current input
    LOG_FATAL,      // print an error message and terminate the program
    LOG_OTHER = -1  // should not be used
} log_lev_t;


/* This enum represents the various types of errors.
 * The error type only affects what is printed to the console. */
typedef enum {
    ERR_APPL,       // application error
    ERR_OS,         // operating system error
    ERR_MALLOC,     // user malloc error
} err_type_t;


/* This function will log information to the console given a log_lev_t enum
 * and a log string. Use it for system level errors or debugging info. Output 
 * created by this function will not affect grading. */
int logging(log_lev_t, char*);
