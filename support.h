/**************************************************************************
 * C S 429 MM-lab
 * 
 * support.h - Provides support commands for errors and traces
 * 
 * Copyright (c) 2021 M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <time.h>

#define MAXLINE     1024 /* max string size */
#define HDRLINES       2 /* number of header lines in a trace file */
#define LINENUM(i) (i+ 1 + HDRLINES) /* cnvt trace request nums to linenums (origin 1) */

/* Represents an allocated block returned by umalloc */
typedef struct {
    void *payload;
    size_t block_size;
    size_t content_val; 
    bool is_allocated;
} allocated_block_t;


/* Characterizes a single trace operation (allocator request) */
typedef struct {
    enum {ALLOC, FREE} type; /* type of request */
    int index;                        /* index for free() to use later */
    int size;                         /* byte size of alloc request */
} traceop_t;

/* Holds the information for one trace file*/
typedef struct {
    int num_ids;         /* number of alloc ids */
    int num_ops;         /* number of distinct requests */
    traceop_t *ops;      /* array of requests */
    allocated_block_t *blocks; /* array of blocks returned by umalloc */
} trace_t;

void appl_error(char *msg);
void malloc_error(int opnum, char *msg);
trace_t *read_trace(char *filename, int verbose);
void free_trace(trace_t *trace);