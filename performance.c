/**************************************************************************
 * C S 429 MM-lab
 * 
 * performance.c - Runs the traces and evaluates the umalloc package for performance
 * 
 * Copyright (c) 2021 M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "umalloc.h"
#include "support.h"

static void run_trace(trace_t *trace) {

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uinit();
    for(size_t curr_op = 0; curr_op < trace->num_ops; curr_op++) {
        if (curr_op % 5 == 0) {
            sbrk(4096);
        }
        traceop_t op = trace->ops[curr_op];
        if (op.type == ALLOC) {
            trace->blocks[op.index].payload = umalloc(op.size);
        } else {
            ufree(trace->blocks[op.index].payload);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Success: %ld", delta_us);
}



int main(int argc, char **argv) { 
    if (argc < 2) {
        fprintf(stderr, "Usage: performance file\n");
        appl_error("No File parameter provided.");
    }
    trace_t *trace = read_trace(argv[1], 0);
    run_trace(trace);
    free_trace(trace);
    return 0;
}