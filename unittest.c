#include "check_heap.h"
#include "csbrk.h"
#include "err_handler.h"
#include "support.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define COMMENT '#'
#define BLANK '\n'
#define SEPARATOR '@'
#define FREE 'f'
#define ALLOC 'a'
#define FIND 'F'
#define EXTEND 'E'
#define SPLIT 'S'
#define COALESCE 'C'
#define MAX_LINE_LENGTH 160

static char printbuf[MAX_LINE_LENGTH];
static char linebuf[MAX_LINE_LENGTH];
static int size_offset;
static bool check;
extern memory_block_t *free_head;

/* A struct for keeping track of test blocks. */
typedef struct block_record {
    uint32_t id;
    memory_block_t *addr;
} record_t;

/* Function interfaces */
static FILE *read_args(int argc, char **argv);
static void list_add(record_t **record_table, uint32_t id);
static void list_remove(record_t **record_table, uint32_t id);
static memory_block_t *initialize_list(void *heap, record_t **record_table, FILE *infile);
static void backup_list(record_t **dest, record_t **source, size_t len);
static void run_tests(record_t **record_table, record_t **backup, size_t len, FILE *infile);

static void print_block(memory_block_t *block);
// static void print_records(record_t **record_table, size_t len);
static void print_list(memory_block_t *free_list);

static void run_heap_check();
static void test_find(size_t size);
static void test_extend(size_t size);
static void test_split(record_t **record_table, uint32_t id, size_t size);
static void test_coalesce(record_t **record_table, uint32_t id);

/* Run all tests */
int main(int argc, char **argv) {

    void *heap = NULL;
    size_offset = 0;
    check = false;

    FILE *infile = read_args(argc, argv);
    linebuf[0] = COMMENT;

    while (linebuf[0] == COMMENT || linebuf[0] == BLANK) {
        if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
            logging(LOG_FATAL, "Could not read from input file.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    size_t heap_size, num_blocks;
    sscanf(linebuf, "%ld %ld", &heap_size, &num_blocks);

    record_t **record_table = (record_t **)calloc(num_blocks, sizeof(record_t *));
    record_t **record_table_copy = (record_t **)calloc(num_blocks, sizeof(record_t *));
    heap = csbrk(heap_size);
    free_head = initialize_list(heap, record_table, infile);

    for (int i = 0; i < num_blocks; i++) {
        record_table_copy[i] = (record_t *)malloc(sizeof(record_t));
        record_table_copy[i]->id = i+1;
        record_table_copy[i]->addr = (memory_block_t *)malloc(sizeof(memory_block_t));
    }
    backup_list(record_table_copy, record_table, num_blocks);
    
    sprintf(printbuf, "Initial free list state:");
    logging(LOG_INFO, printbuf);
    print_list(free_head);

    run_heap_check();

    run_tests(record_table, record_table_copy, num_blocks, infile);
    return EXIT_SUCCESS;
}

static FILE *read_args(int argc, char **argv) {
    int option;
    FILE *infile = NULL;

    while ((option = getopt(argc, argv, ":i:s:c")) != -1) {
        switch(option) {
            case 'i':
                if ((infile = fopen(optarg, "r")) == NULL) {
                    sprintf(printbuf, "input file %s not found", optarg);
                    logging(LOG_FATAL, printbuf);
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                size_offset = sizeof(memory_block_t);
                break;
            case 'c':
                check = true;
                break;
            default:
                sprintf(printbuf, "Ignoring unknown option %c", optopt);
                logging(LOG_INFO, printbuf);
                break;
        }
    }
    if (!infile) {
        sprintf(printbuf, "No input file provided.\n");
        logging(LOG_FATAL, printbuf);
        exit(EXIT_FAILURE);
    }
    return infile;
}

static void list_add(record_t **record_table, uint32_t id) {
    memory_block_t *block = NULL;
    for (int i = 1; i < id; i++) {
        if (!is_allocated(record_table[i-1]->addr)) {
            block = record_table[i-1]->addr;
        }
    }
    if (block) block->next = record_table[id-1]->addr;
}

static void list_remove(record_t **record_table, uint32_t id) {
    memory_block_t *block = NULL;
    for (int i = 1; i < id; i++) {
        if (!is_allocated(record_table[i-1]->addr)) {
            block = record_table[i-1]->addr;
        }
    }
    if (block) block->next = record_table[id-1]->addr->next;
}

static memory_block_t *initialize_list(void *heap, record_t **record_table, FILE* infile) {
    memory_block_t *free_list = NULL;
    char op;
    uint32_t id;
    size_t size;
    int id_counter = 0;
    size_t total_size = 0;

    if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
        logging(LOG_FATAL, "Could not read from input file.\n");
        exit(EXIT_FAILURE);
    }

    while (linebuf[0] != SEPARATOR) {
        if (linebuf[0] == COMMENT || linebuf[0] == BLANK) {
            if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
                logging(LOG_FATAL, "Could not read from input file.\n");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        sscanf(linebuf, "%c %d %ld", &op, &id, &size);
        memory_block_t *block;

        if (id > id_counter) {
            id_counter++;
            block = (memory_block_t *)(heap + total_size);
            total_size += size + sizeof(memory_block_t);
            /* ids are 1-indexed. */
            record_table[id-1] = (record_t *)malloc(sizeof(record_t));
            record_table[id-1]->id = id;
            record_table[id-1]->addr = block;
        }
        else {
            block = record_table[id-1]->addr;
        }
        
        switch (op) {
            case ALLOC:
                list_remove(record_table, id);
                put_block(block, size + size_offset, true);
                break;
            case FREE:
                list_add(record_table, id);
                put_block(block, size + size_offset, false);
                break;
            default:
                break;
        }

        if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
            logging(LOG_FATAL, "Could not read from input file.\n");
            exit(EXIT_FAILURE);
        }
    }

    free_list = record_table[0]->addr;
    for (int i = 0; is_allocated(free_list); i++) {
        free_list = record_table[0]->addr;
    }

    return free_list;
}

static void backup_list(record_t **dest, record_t **source, size_t len) {
    for (int i = 0; i < len; i++) {
        memcpy(dest[i]->addr, source[i]->addr, sizeof(memory_block_t));
    }
}

static void run_tests(record_t **record_table, record_t **backup, size_t len, FILE *infile) {
    char op;
    uint32_t id;
    size_t size;

    if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
        logging(LOG_FATAL, "Could not read from input file.\n");
        exit(EXIT_FAILURE);
    }

    while (linebuf[0] != SEPARATOR) {
        if (linebuf[0] == COMMENT || linebuf[0] == BLANK) {
            if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
                logging(LOG_FATAL, "Could not read from input file.\n");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        switch (linebuf[0]) {
            case FIND:
                sscanf(linebuf, "%c %ld", &op, &size);
                test_find(size);
                break;
            case EXTEND:
                sscanf(linebuf, "%c %ld", &op, &size);
                test_extend(size);
                break;
            case SPLIT:
                sscanf(linebuf, "%c %d %ld", &op, &id, &size);
                test_split(record_table, id, size);
                break;
            case COALESCE:
                sscanf(linebuf, "%c %d", &op, &id);
                test_coalesce(record_table, id);
                break;
            default:
                break;
        }

        run_heap_check();
        backup_list(record_table, backup, len);

        if (fgets(linebuf, sizeof(linebuf), infile) == NULL) {
            logging(LOG_FATAL, "Could not read from input file.\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void print_block(memory_block_t *block) {
    sprintf(printbuf, "node at %p has size %ld, allocated status of %d, and next of %p", 
        block, get_size(block), is_allocated(block), get_next(block));
    logging(LOG_INFO, printbuf);
}

// static void print_records(record_t **record_table, size_t len) {
//     for (int i = 0; i < len; i++) {
//         sprintf(printbuf, "Record with ID %d has ", record_table[i]->id);
//         logging(LOG_INFO, printbuf);
//         print_block(record_table[i]->addr);
//     }
// }

static void print_list(memory_block_t *head) {
    while(head) {
        print_block(head);
        head = head->next;
    }
    sprintf(printbuf, "End of free list.\n");
    logging(LOG_INFO, printbuf);
}

static void run_heap_check() {
    if (check) {
        sprintf(printbuf, "Running heap checker...");
        logging(LOG_INFO, printbuf);
        int check_heap_status = check_heap();
        if (check_heap_status) {
            sprintf(printbuf, "Heap checker returned nonzero status: %d", check_heap_status);
            logging(LOG_ERROR, printbuf);
        }
        else {
            sprintf(printbuf, "Heap checker returned all good!");
            logging(LOG_INFO, printbuf);
        }
        
    }
}

static void test_find(size_t size) {
    sprintf(printbuf, "Testing find with a size of %ld:", size);
    logging(LOG_INFO, printbuf);

    memory_block_t *block = find(size);
    if (!block) {
        sprintf(printbuf, "Find returned NULL. This may be intentional.\n");
        logging(LOG_WARNING, printbuf);
    }
    else if (get_size(block) >= size) {
        sprintf(printbuf, "Find returned a block with size %ld.\n", get_size(block));
        logging(LOG_INFO, printbuf);
    }
    else {
        sprintf(printbuf, "Find returned a block with size %ld.\n", get_size(block));
        logging(LOG_ERROR, printbuf);
    }
}

static void test_extend(size_t size) {
    sprintf(printbuf, "Testing extend with a size of %ld:", size);
    logging(LOG_INFO, printbuf);

    memory_block_t *block = extend(size);
    if (!block) {
        sprintf(printbuf, "Extend returned NULL.\n");
        logging(LOG_ERROR, printbuf);
    }
    else if (get_size(block) >= size) {
        sprintf(printbuf, "Extend returned a block with size %ld.\n", get_size(block));
        logging(LOG_INFO, printbuf);
    }
    else {
        sprintf(printbuf, "Extend returned a block with size %ld.\n", get_size(block));
        logging(LOG_ERROR, printbuf);
    }
}

static void test_split(record_t **record_table, uint32_t id, size_t size) {
    memory_block_t *block = record_table[id-1]->addr;
    size_t original_size = get_size(block);
    memory_block_t *original_next = block->next;

    sprintf(printbuf, "Testing split on a block with an initial size of %ld:", get_size(block));
    logging(LOG_INFO, printbuf);
    sprintf(printbuf, "Target split size is %ld", size);
    logging(LOG_INFO, printbuf);

    memory_block_t *split_block = split(block, size);
    
    if (!split_block) {
        sprintf(printbuf, "Split returned NULL.\n");
        logging(LOG_WARNING, printbuf);
    }
    else if (original_size <= size+2*sizeof(memory_block_t)-size_offset) {
        if (get_size(split_block) == original_size && split_block->next == original_next) {
            sprintf(printbuf, "Block was not split.\n");
            logging(LOG_INFO, printbuf);
        }
        else {
            sprintf(printbuf, "Block was modified when it should not have split.\n");
            logging(LOG_ERROR, printbuf);
        }
    }
    else {
        
        if (get_size(split_block) == original_size && split_block->next == original_next) {
            sprintf(printbuf, "Block was not split.\n");
            logging(LOG_WARNING, printbuf);
            return;
        }
        else if (block == split_block) {
            sprintf(printbuf, "Block was split.");
            logging(LOG_INFO, printbuf);
            size_t alloc_size = get_size(split_block);
            split_block = (memory_block_t *)((char *)split_block + sizeof(memory_block_t) - size_offset + alloc_size);
            size_t new_size = get_size(split_block);
            if (alloc_size >= ALIGN(size) && new_size + alloc_size + sizeof(memory_block_t) - size_offset == original_size) {
                sprintf(printbuf, "New sizes: %ld free, %ld allocated.\n", new_size, alloc_size);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "New sizes: %ld free, %ld allocated.\n", new_size, alloc_size);
                logging(LOG_ERROR, printbuf);
            }
        }
        else {
            sprintf(printbuf, "Block was split.");
            logging(LOG_INFO, printbuf);
            size_t alloc_size = get_size(split_block);
            split_block = (memory_block_t *)((char *)(split_block) - original_size + alloc_size);
            size_t new_size = get_size(split_block);
            if (alloc_size >= ALIGN(size) && new_size + alloc_size + sizeof(memory_block_t) - size_offset == original_size) {
                sprintf(printbuf, "New sizes: %ld free, %ld allocated.", new_size, alloc_size);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "New sizes: %ld free, %ld allocated.", new_size, alloc_size);
                logging(LOG_ERROR, printbuf);
            }
        }
        sprintf(printbuf, "Original next block: %p, new next block: %p.\n", original_next, get_next(split_block));
        logging(LOG_INFO, printbuf);
    }
}

static void test_coalesce(record_t **record_table, uint32_t id) {
    memory_block_t *block = record_table[id-1]->addr;
    size_t original_size = get_size(block);
    memory_block_t *original_next = get_next(block);

    memory_block_t *prev = record_table[id-2]->addr;
    bool can_coalesce_left = !is_allocated(prev);
    memory_block_t *next = record_table[id]->addr;
    bool can_coalesce_right = !is_allocated(next);

    size_t target_size = original_size;
    if (can_coalesce_left) {
        target_size += get_size(prev) + sizeof(memory_block_t) - size_offset;
    }
    if (can_coalesce_right) {
        target_size += get_size(next) + sizeof(memory_block_t) - size_offset;
    }

    sprintf(printbuf, "Testing coalesce on a block with an initial size of %ld:", get_size(block));
    logging(LOG_INFO, printbuf);

    memory_block_t *coalesced_block = coalesce(block);
    if (!coalesced_block) {
        sprintf(printbuf, "Coalesce returned NULL.\n");
        logging(LOG_WARNING, printbuf);
    }
    else {
        size_t new_size = get_size(coalesced_block);
        memory_block_t *new_next = get_next(coalesced_block);

        if (can_coalesce_left && can_coalesce_right) {
            sprintf(printbuf, "Could have coalesced both left and right.");
            logging(LOG_INFO, printbuf);
            if (new_size == target_size) {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_WARNING, printbuf);
            }
            if (coalesced_block == prev) {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_ERROR, printbuf);
            }
        }
        else if (can_coalesce_left) {
            sprintf(printbuf, "Could have coalesced left.");
            logging(LOG_INFO, printbuf);
            if (new_size == target_size) {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_WARNING, printbuf);
            }
            if (coalesced_block == prev) {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_ERROR, printbuf);
            }
        }
        else if (can_coalesce_right) {
            sprintf(printbuf, "Could have coalesced right.");
            logging(LOG_INFO, printbuf);
            if (new_size == target_size) {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Size after coalescing: %ld", new_size);
                logging(LOG_WARNING, printbuf);
            }
            if (coalesced_block == block) {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Block address after coalescing: %p\n", coalesced_block);
                logging(LOG_ERROR, printbuf);
            }
        }
        else {
            sprintf(printbuf, "Could not have coalesced.");
            logging(LOG_INFO, printbuf);
            if (new_size == target_size && new_next == original_next) {
                sprintf(printbuf, "Did not attempt to coalesce.\n");
                logging(LOG_INFO, printbuf);
            }
            else {
                sprintf(printbuf, "Tried coalescing when not possible.\n");
                logging(LOG_WARNING, printbuf);
            }
        }
    }
}