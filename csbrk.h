/**************************************************************************
 * C S 429 MM-lab
 * 
 * csbrk.h - A wrapper for sbrk system call. Used to keep track of calls
 * and introduce an upper limit to the amount of memory one can request at any
 * one time.
 * 
 * Copyright (c) 2021 M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#define PAGESIZE 4096

void *csbrk(intptr_t increment);
int check_malloc_output(void *payload_start, size_t payload_length);