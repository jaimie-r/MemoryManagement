/**************************************************************************
 * C S 429 MM-lab
 * 
 * runner.c - Runs the traces and evaluates the umalloc package for correctness
 * and utilization.
 * 
 * Copyright (c) 2021 M. Hinton. All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "csbrk.h"
#include "support.h"
#include "check_heap.h"
#include <sys/mman.h>

int verbose = 0;
char msg[MAXLINE];      /* for whenever we need to compose an error message */
extern size_t sbrk_bytes;
extern const char author[];

/* 
 * usage - Explain the command line arguments
 */
static void usage(void) 
{
    fprintf(stderr, "Usage: mdriver [-rhvuc] file\n");
    fprintf(stderr, "Options\n");
    fprintf(stderr, "\t-r         Run the trace to completion (bypass interface).\n");
    fprintf(stderr, "\t-h         Print this message.\n");
    fprintf(stderr, "\t-v         Print additional debug info.\n");
    fprintf(stderr, "\t-u         Display heap utilization.\n");
    fprintf(stderr, "\t-c         Runs the user provided heap check after every op.\n");
}

/* 
 * copy_id - Writes the block id out to the payload. To be used for correctness
 * checks.
 */
static void copy_id(size_t *block, size_t block_size, size_t id) {
    size_t words = block_size/ sizeof(size_t);
    for(size_t i = 0; i < words; i++) {
        block[i] = id;
    }
}

/* 
 * check_id - Checks the block contains the block id, repeated the number of
 * words can fit.
 */
static int check_id(size_t *block, size_t block_size, size_t id) {
    size_t words = block_size/ sizeof(size_t);
    for(size_t i = 0; i < words; i++) {
        if (block[i] != id) {
            return -1;
        }
    }

    return 0;
}

/* 
 * check_correctness - Checks if every block that is mark allocated has the 
 * correct id written out. If this fails, means that an allocated payload
 * was affected by the umalloc package. 
 */
static int check_correctness(trace_t *trace, size_t curr_op) {
    for (size_t block_id = 0; block_id < trace->num_ids; block_id++) {
        allocated_block_t *block = &trace->blocks[block_id];
        if (block->is_allocated) {
            if (check_id(block->payload, block->block_size, block->content_val) == -1) {
                sprintf(msg, "umalloc corrupted block id %lu.", block_id);
                malloc_error(curr_op, msg);
                return -1;
            }
        }
    }

    if (verbose) {
        printf("line %ld passed the correctness check.\n", LINENUM(curr_op));
    }

    return 0;
}

size_t curr_bytes_in_use;
size_t max_bytes_in_use;

/* 
 * UTILIZATION_SCORE - the utilization score represents how well the umalloc
 * package uses the bytes requested from sbrk. For example, if 100 bytes are
 * requested from sbrk, and the user requested 80 bytes, there will be a 
 * utilization score of 80%.
 */
#define UTILIZATION_SCORE 100.0 * max_bytes_in_use / sbrk_bytes

/* 
 * run_trace_line - Runs a single line in the trace. Checking if all the 
 * correctness checks are still satisfied after the check. Checks if the returned
 * payload is aligned to 16 bytes, hasn't affected any other blocks, and rests
 * within the sbrk range. Runs the user created check heap function and prints
 * the current utilization score if requested. 
 */
static int run_trace_line(trace_t *trace, size_t curr_op, int utilization, int run_check_heap) {

    if (curr_op % 5 == 0) {
        void *ret = sbrk(4096);
        mprotect(ret, 4096, PROT_NONE);
    }
    traceop_t op = trace->ops[curr_op];
    if (op.type == ALLOC) {
        trace->blocks[op.index].is_allocated = true;
        trace->blocks[op.index].content_val = curr_op;
        trace->blocks[op.index].block_size = op.size;

        if (verbose) {
            printf("line %ld: umalloc: id %d, Allocating %d bytes\n", LINENUM(curr_op), op.index, op.size);
        }

        trace->blocks[op.index].payload = umalloc(op.size);
        curr_bytes_in_use += op.size;
        if ( trace->blocks[op.index].payload == NULL) {
            malloc_error(curr_op, "umalloc failed.");
            return -1;
        }

        if (((size_t)trace->blocks[op.index].payload) % ALIGNMENT != 0) {
            malloc_error(curr_op, "umalloc returned an unaligned payload.");
            return -1;
        }

        if(check_malloc_output(trace->blocks[op.index].payload, trace->blocks[op.index].block_size) == -1) {
            printf("line %ld: umalloc allocated a block out of bounds.\n", LINENUM(curr_op));
            return -1;
        }

        copy_id((size_t*) trace->blocks[op.index].payload, trace->blocks[op.index].block_size, curr_op);
    } else {
        trace->blocks[op.index].is_allocated = false;

        if (verbose) {
            printf("line %ld: ufree: id %d\n", LINENUM(curr_op), op.index);
        }

        ufree(trace->blocks[op.index].payload);
        curr_bytes_in_use -= trace->blocks[op.index].block_size;
    }

    if (curr_bytes_in_use > max_bytes_in_use) {
        max_bytes_in_use = curr_bytes_in_use;
    }

    if (run_check_heap) {
        if (check_heap() != 0) {
            malloc_error(curr_op, "check heap failed.");
            return -1;
        } else {
            printf("Passed check heap.\n");
        }
    }

    if (check_correctness(trace, curr_op) == -1) {
        printf("line %ld failed the correctness check.\n", LINENUM(curr_op));
        return -1;
    }

    if (verbose && utilization) {
        printf("Current Utilization percentage: %.2f\n", UTILIZATION_SCORE);
    }

  return 0;
}

/* 
 * auto_run_trace - Starting from curr_op, runs the trace to completetion. 
 * Printing the utlilization and running check_heap if requested. 
 */
static int auto_run_trace(trace_t *trace, int utilization, int run_check_heap, size_t curr_op) {

    if (curr_op >= trace->num_ops) {
        printf("Trace run to completetion.\n");
        return curr_op;
    }

    for(;curr_op < trace->num_ops; curr_op++) {
        if (run_trace_line(trace, curr_op, utilization, run_check_heap) == -1) {
            printf("umalloc package failed.\n");
            exit(1);
        }
    }

    printf("umalloc package passed correctness check.\n");

    if (utilization) {
        printf("Final Utilization percentage: %.2f\n", UTILIZATION_SCORE);
    }
    return curr_op;
}

/* 
 * help - Prints the help information for the Trace Runner.
 */
void help() {                                                    
    printf("----------------MSIM Help-----------------------\n");
    printf("go               -  run trace to completion         \n");
    printf("run n            -  execute trace for n ops\n");
    printf("check            -  run the heap_check                \n");
    printf("util             -  display current heap utilization   \n");
    printf("help             -  display this help menu            \n");
    printf("quit             -  exit the program                  \n\n");
}

/* 
 * interactive_run_trace - Interactive mode to run a trace, supported commands
 * provided in the help function.
 */
void interactive_run_trace(trace_t *trace, int utilization, int run_check_heap) {                         
  char buffer[20];
  int ops_to_run;
  int ret;
  size_t curr_op = 0;

  while(1) {
    printf("MM> ");

    int size = scanf("%s", buffer);
    if (size == 0) {
        buffer[0] = 's';
    }
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
        curr_op = auto_run_trace(trace, utilization, run_check_heap, curr_op);
        break;
    
    case 'C':
    case 'c':
        printf("Running check_heap.\n");
        ret = check_heap();
        if (ret != 0)
            printf("check_heap returned non zero exit code.\n");
        break;

    case 'h':
    case 'H':
        help();
        break;

    case 'Q':
    case 'q':
        printf("Bye.\n");
        exit(0);

    case 'U':
    case 'u':
        printf("Current Utilization percentage: %.2f\n", UTILIZATION_SCORE);
        break;

    case 'R':
    case 'r':
        size = scanf("%d", &ops_to_run);
        if (size == 0) {
            break;
        }

        if (curr_op >= trace->num_ops) {
            printf("Trace run to completetion.\n");
            break;
        }

        for(int op = 0; op < ops_to_run; op++) {
            if (run_trace_line(trace, curr_op, utilization, run_check_heap) == -1) {
                printf("umalloc package failed.\n");
                exit(1);
            }
            curr_op++;
            if (curr_op == trace->num_ops) {
                printf("umalloc package passed correctness check.\n");
                break;
            }
        }

        if (utilization && curr_op >= trace->num_ops) {
            printf("Final Utilization percentage: %.2f\n", UTILIZATION_SCORE);
        }

        break;

    default:
        printf("Invalid Command\n");
        break;
    }
  }
}


int main(int argc, char **argv)
{

  char c;
  int autorun = 0, run_check_heap = 0, display_utilization = 0;

  /* 
    * Read and interpret the command line arguments 
    */
  while ((c = getopt(argc, argv, "rvhcu")) != EOF) {
    switch (c) {
    case 'r': /* Generate summary info for the autograder */
        autorun = 1;
        break;
    case 'v': /* Print per-trace performance breakdown */
        verbose = 1;
        break;
    case 'h': /* Print this message */
        usage();
        exit(0);
    case 'c':
        run_check_heap = 1;
        break;
    case 'u':
        display_utilization = 1;
        break;
    default:
        usage();
        exit(1);
    }
    }

    char *file = argv[optind];

    if (file == NULL) {
        usage();
        appl_error("Missing file parameters.");
    }

    if (verbose) {
        if (autorun) {
            printf("Auto Run Enabled.\n");
        }

        if (display_utilization) {
            printf("Displaying Utilization.\n");
        }

        if (run_check_heap) {
           printf("Running Check Heap After Each Op.\n");
        }
    }

    printf("Welcome to the MM lab runner\n\n");
    printf("Author: %s\n", author);

    trace_t *trace = read_trace(file, verbose);
    if (uinit() == -1) {
        malloc_error(-3, "uinit failed.");
        exit(1);
    }
    curr_bytes_in_use = 0;
    max_bytes_in_use = 0;
    if (autorun) {
        auto_run_trace(trace, display_utilization, run_check_heap, 0);
    } else {
        interactive_run_trace(trace, display_utilization, run_check_heap);
    }
    free_trace(trace);
}