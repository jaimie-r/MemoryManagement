#include "umalloc.h"
#include "csbrk.h"
#include <stdio.h>
#include <assert.h>
#include "ansicolors.h"

const char author[] = ANSI_BOLD ANSI_COLOR_RED "JAIMIE REN JLR6866" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the (payload) size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    //* STUDENT TODO
    int blockSize = get_size(free_head);
    memory_block_t *cur = free_head;
    memory_block_t *res = NULL;
    bool minimize = false;
    while(cur) {
        int curSize = get_size(cur);
        if(curSize >= size) {
            if(!minimize) {
                res = cur;
                blockSize = curSize;
                minimize = true;
            } else {
                if(curSize < blockSize) {
                    res = cur;
                    blockSize = curSize;
                }
            }
        }
        cur = cur->next;
    }
    return res;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //* STUDENT TODO
    memory_block_t *res = csbrk(size + ALIGNMENT);
    res->block_size_alloc = size;
    if(res < free_head) {
        res->next = free_head;
        free_head = res;
        return res; //coalesce
    } else {
        memory_block_t *cur = free_head;
        while(cur->next) {
            cur = cur->next;
        }
        cur->next = res;
    }
    return res; // coalesce
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) { // get a size sized block from block
    //* STUDENT TODO
    int fullBlockSize = block->block_size_alloc;
    while(size % ALIGNMENT != 0) {
        size++;
    }
    block->block_size_alloc = size;
    memory_block_t *prev = free_head;
    if(block == prev) {
        if(prev->next) {
            prev->next = prev->next->next;
        } else {
            prev->next = NULL;
        }
    } else {
        bool done = false;
        while(prev->next && !done) {
            if(prev->next == block) {
                prev->next = prev->next->next;
                done = true;
            }
            if (!done) {
                prev = prev->next;
            }
        }
    }
    allocate(block);
    memory_block_t *free = (memory_block_t *)((char *)block + size + ALIGNMENT);
    free->block_size_alloc = fullBlockSize - size;
    deallocate(free);
    if(block == free_head) {
        free_head = free;
    }
    ufree(free);
    return block; // don't split
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
// called coalesce at the end of extend, check if the extended heap is connected to original heap
memory_block_t *coalesce(memory_block_t *block) {
    //* STUDENT TODO
    memory_block_t *prev = free_head;
    memory_block_t *res = block;
    int blockEnd = ((uintptr_t)block + block->block_size_alloc + ALIGNMENT);
    if(block == prev) { // coalescing at beginning
        if(prev->next) {
            if(blockEnd == (uintptr_t)prev->next) { // first and second are adjacent
                block->block_size_alloc += prev->next->block_size_alloc + ALIGNMENT;
                block->next = prev->next->next;
                
            }
        }
    } else {
        while(prev->next) {
            if(prev->next == block) { // found the free block
                res = NULL;
                memory_block_t *prevEnd = (memory_block_t *)(uintptr_t)prev + prev->block_size_alloc + ALIGNMENT;
                if(block->next && blockEnd == (uintptr_t)block->next) { // coalesce with next block
                    block->block_size_alloc += block->next->block_size_alloc + ALIGNMENT;
                    block->next = block->next->next;
                    res = block;
                }
                if(prevEnd == block) { // coalesce with prev
                    prev->block_size_alloc += block->block_size_alloc + ALIGNMENT;
                    prev->next = block->next;
                    res = prev;
                }
                return res;
            }
            prev = prev->next;
        }
    }
    return res;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    //* STUDENT TODO
    // use csbrk, any mulitple of pagesize
    int size = 5 * PAGESIZE;
    void *ptr = csbrk(size);
    if(!ptr) {
        return -1;
    }
    free_head = ptr;
    free_head->block_size_alloc = size - ALIGNMENT;
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    //* STUDENT TODO
    // call find to get free block
    memory_block_t *bptr = find(size);
    if(!bptr) { // didn't find a block big enough
        bptr = extend(5 * PAGESIZE); // extend adds to free list and coalesces
    }
    if(bptr->block_size_alloc - size >= (ALIGNMENT * 3)) { // split
        return split(bptr, size);
    }
    // allocating entire block so fix the free list
    memory_block_t *prev = free_head;
    if(prev == bptr) { // allocated free_head
        free_head = free_head->next;
    } else {
        bool done = false;
        while(prev->next) {
            if(prev->next == bptr && !done) { // found bptr
                prev->next = prev->next->next; // bridging free list
                done = true;
            }
            prev = prev->next;
        }
    }
    // after free list is fixed, allocate the block
    allocate(bptr);
    return bptr;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //* STUDENT TODO
    memory_block_t *bptr = (memory_block_t *) (ptr);
    deallocate(bptr);
    memory_block_t *prev = free_head;
    if(prev > bptr) { // new free_head
        bptr->next = free_head;
        free_head = bptr;
        coalesce(bptr);
    } else {
        while(prev->next) {
            if(prev->next > bptr) { // insert free block after prev
                bptr->next = prev->next;
                prev->next = bptr;
                coalesce(bptr);
                return;
            }
            prev = prev->next;
        }
        if(bptr > prev) { // add to the end of the list
            prev->next = bptr;
        }
        coalesce(bptr);
    }

}