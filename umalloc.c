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
 * get_size - gets the size of the block.
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
    while(cur) {
        int curSize = get_size(cur);
        if(curSize >= size && curSize < blockSize) {
            res = cur;
            blockSize = curSize;
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
        return coalesce(res);
    } else {
        memory_block_t *cur = free_head;
        while(cur->next) {
            cur = cur->next;
        }
        cur->next = res;
    }
    return coalesce(res);
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) { // get a size sized block from block
    //* STUDENT TODO
    if(block->block_size_alloc - size >= 24) { // split
        int fullBlockSize = block->block_size_alloc;
        memory_block_t *res = block;
        res->block_size_alloc = size;
        allocate(res);
        memory_block_t *free;
        free = block + fullBlockSize + ALIGNMENT;
        free->block_size_alloc = fullBlockSize - size;
        ufree(free);
        return res;
    }
    return block; // don't split
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
// called coalesce at the end of extend, check if the extended heap is connected to original heap
memory_block_t *coalesce(memory_block_t *block) {
    //* STUDENT TODO
    memory_block_t *prev = free_head;
    memory_block_t *blockEnd = block + block->block_size_alloc + ALIGNMENT;
    if(block == prev) { // coalescing at beginning
        if(prev->next) {
            if(!is_allocated(prev->next) && blockEnd == prev->next) { // block and next block are adjacent
                block->block_size_alloc += prev->next->block_size_alloc + ALIGNMENT;
                block->next = prev->next->next;
            }
        }
    } else {
        while(prev->next) {
            if(prev->next == block) { // found the free block
                memory_block_t *prevEnd = prev + prev->block_size_alloc + ALIGNMENT;
                if(!is_allocated(prev) && prevEnd == block) { // prev is back to back with block
                    if(block->next && blockEnd == block->next && !is_allocated(block->next)) { // block is back to back with next so coalesce both sides
                        prev->block_size_alloc += block->block_size_alloc + block->next->block_size_alloc + (2 * ALIGNMENT);
                        prev->next = block->next;
                        return prev;
                    }  
                    // only coalesce with prev
                    prev->block_size_alloc += block->block_size_alloc + ALIGNMENT;
                    prev->next = block->next;
                    return prev;
                }
                if(block->next && !is_allocated(block->next) && blockEnd == block->next) { // only coalesce with next block
                    block->block_size_alloc += block->next->block_size_alloc + ALIGNMENT;
                    block->next = block->next->next;
                    return block;
                }
            }
            prev = prev->next;
        }
    }
    return block;
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
    // 
    void *bptr = find(size);
    if(bptr) { // found a block
        allocate(bptr);
        if(bptr == free_head) { // allocated block is free_head
            free_head = free_head->next;
            return split(bptr, size);
        }
        memory_block_t *prev = free_head;
        while(prev->next) { // allocated block is from middle of list
            if(prev->next == bptr) {
                prev->next = prev->next->next;
                return split(bptr, size);
            }
        }
        prev->next = NULL; // allocated block is last free block
        return split(bptr, size);
    }
    bptr = extend(5 * PAGESIZE);
    return split(bptr, size);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //* STUDENT TODO
    // mask and shift bits to get allocated or free indicator bit to check it's allocated 
    // ^noo use helper methods
    // loop through linked list and get previous and next block
    // check if either is free
    // coalesce (can I call coalesce in here?)
    memory_block_t *bptr = get_block(ptr);
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
        prev->next = bptr;
        bptr->next = NULL;
        coalesce(bptr);
    }

}