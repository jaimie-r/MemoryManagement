/**************************************************************************
 * C S 429 MM-lab
 * 
 * err_handler.c - The error-handling module.
 * 
 * Copyright (c) 2021. S. Chatterjee, X. Shen, T. Byrd, M. Hinton. 
 * All rights reserved. May not be used, modified, or copied without permission.
 **************************************************************************/ 

#include "ansicolors.h"
#include "err_handler.h"
#include <stdio.h>

static char printbuf[1024];

static char *sevnames[LOG_FATAL+1] = {
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};

static char *sevcolors[LOG_FATAL+1] = {
    ANSI_COLOR_CYAN,
    ANSI_COLOR_YELLOW,
    ANSI_COLOR_RED,
    ANSI_BOLD ANSI_COLOR_RED
};

static char* format_log_message(log_lev_t sev, char *msg) {
    sprintf(printbuf, "%s\t[%s] %s" ANSI_RESET, sevcolors[sev], sevnames[sev], msg);
    return printbuf;
}

int logging(log_lev_t sev, char* msg) {
    return fprintf(stderr, "%s\n", format_log_message(sev, msg));
}