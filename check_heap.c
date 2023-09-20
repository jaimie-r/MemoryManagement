
#include "umalloc.h"
#include <stdio.h>
#include <stdint.h>

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 *
 * STUDENT TODO:
 * Required to be completed for checkpoint 1:
 *
 *      - Ensure that the free block list is in the order you expect it to be in.
 *        As an example, if you maintain the free list in memory-address order,
 *        lowest address first, ensure that memory addresses strictly ascend as you
 *        traverse the free list.
 *
 *      - Check if any free memory_blocks overlap with each other. 
 *
 *      - Ensure that each memory_block is aligned. 
 * 
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    // Example heap check:
    // Check that all blocks in the free list are marked free.
    // If a block is marked allocated, return -1.
    memory_block_t *cur = free_head;
    while (cur) {
        printf("block address: %p\n", cur);
        printf("block_size_alloc: %d\n", (int)cur->block_size_alloc);
        printf("next: %p\n", cur->next);
        printf("\n");
        if (is_allocated(cur)) { // marked free
            return -1;
        }
        if(cur->next) {
            if(&cur > &cur->next) { // address-sorted
                return -2;
            }
            // if memory address + size >= next memory address return -1
            if( (cur + get_size(cur) ) >= cur->next ) { // checking for overlap
                return -3;
            }
        }
        unsigned int curNum = (uintptr_t) cur;
        if( curNum % ALIGNMENT != 0) { // 16-byte aligned
            return -4;
        }
        cur = cur->next;
    }
    return 0;
}