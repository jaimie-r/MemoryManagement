/**************************************************************************
 * C S 429 MM-lab
 * 
 * support.c - Provides support commands for errors and traces
 * 
 * Copyright (c) 2021 M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "support.h"
#include "err_handler.h"

char msg[MAXLINE];      /* for whenever we need to compose an error message */

/* 
 * appl_error - Report an arbitrary application error
 * This Can include either failed calls to Malloc
 * or invalid user input
 */
void appl_error(char *msg) 
{
    logging(LOG_FATAL, msg);
    exit(1);
}

/*
 * malloc_error - Report an error returned by the umalloc package
 */
void malloc_error(int opnum, char *msg)
{
    char err_msg[MAXLINE];
    sprintf(err_msg, "[line %d]: %s", LINENUM(opnum), msg);
    logging(LOG_ERROR, err_msg);
}

/*
 * read_trace - read a trace file and store it in memory
 */
trace_t *read_trace(char *filename, int verbose)
{
    FILE *tracefile;
    trace_t *trace;
    char type[MAXLINE];
    int err;

    if (verbose)
        printf("Reading tracefile: %s\n", filename);

    /* Allocate the trace record */
    if ((trace = (trace_t *) malloc(sizeof(trace_t))) == NULL)
        appl_error("malloc 1 failed in read_trace");

    /* Read the trace file header */
    if ((tracefile = fopen(filename, "r")) == NULL) {
        sprintf(msg, "Could not open %s in read_trace", filename);
        appl_error(msg);
    }

    err = fscanf(tracefile, "%d", &(trace->num_ids)); 
    if (err == EOF) {
        appl_error("fscanf failed to find num ids.");
    }

    err = fscanf(tracefile, "%d", &(trace->num_ops)); 
    if (err == EOF) {
        appl_error("fscanf failed to find num ops.");
    }    
    
    /* We'll store each request line in the trace in this array */
    trace->ops = (traceop_t *)calloc(trace->num_ops, sizeof(traceop_t));
    if (trace->ops == NULL)
        appl_error("Failed to allocate op array");

    /* We'll keep an array of pointers to the allocated blocks here... */
    trace->blocks = (allocated_block_t *)calloc(trace->num_ids, sizeof(allocated_block_t));
    if (trace->blocks == NULL)
        appl_error("Failed to allocate block array");

    
    /* read every request line in the trace file */
    unsigned index = 0;
    unsigned op_index = 0;
    unsigned max_index = 0;
    unsigned size = 0;
    while (fscanf(tracefile, "%s", type) != EOF) {
        switch(type[0]) {
        case 'a':
            err = fscanf(tracefile, "%u %u", &index, &size);
            if (err == EOF) {
                appl_error("fscanf failed to find index and size.");
            }
            trace->ops[op_index].type = ALLOC;
            trace->ops[op_index].index = index;
            trace->ops[op_index].size = size;
            max_index = (index > max_index) ? index : max_index;
            break;
        case 'f':
            err = fscanf(tracefile, "%ud", &index);
            if (err == EOF) {
                appl_error("fscanf failed to find index.");
            }
            trace->ops[op_index].type = FREE;
            trace->ops[op_index].index = index;
        break;
        default:
            sprintf(msg, "Bogus type character (%c) in tracefile %s\n", type[0], filename);
            appl_error(msg);
        }
        op_index++;

    }
    fclose(tracefile);
    assert(max_index == trace->num_ids - 1);
    assert(trace->num_ops == op_index);

    return trace;
}

/*
 * free_trace - Free the trace record and the two arrays it points
 *              to, all of which were allocated in read_trace().
 */
void free_trace(trace_t *trace)
{
    free(trace->ops);         /* free the two arrays... */
    free(trace->blocks);      
    free(trace);              /* and the trace record itself... */
}