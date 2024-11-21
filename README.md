### PROJECT DESCRIPTION

## Objectives
Implement a dynamic memory allocator (umalloc and ufree) that behaves correctly, efficiently, and with high throughput.
Design a heap consistency checker to validate the integrity of the heap during memory allocation and deallocation.
Explore and implement different memory management strategies, such as splitting and coalescing free blocks.
Optimize the allocator's performance and minimize memory fragmentation.
Validate the allocator with unit tests and a trace-driven driver program to ensure correctness and performance.

The core functionality will be encapsulated in three functions:

1. uinit: This function initializes the allocator, setting up any required memory regions before memory allocation and deallocation operations can occur. The return value will be 0 for successful initialization, and -1 if any errors occur.

2. umalloc: The memory allocation function, which returns a pointer to a block of memory of at least the requested size, ensuring that the memory is properly aligned (16-byte alignment). It will not overlap with any other allocated blocks and will be within the heap region.

3. ufree: The deallocation function, which frees a block of memory previously allocated by umalloc. This function will not return any value and will only operate on memory that was previously allocated using umalloc and has not already been freed.

## Key Features
Heap Consistency: A heap checker will be implemented to ensure the integrity of the heap. This checker will verify that the free list is correctly ordered, that no free memory blocks overlap, and that all memory blocks are 16-byte aligned. Additional checks may include verifying that each block in the free list is marked as free and ensuring that memory obtained from the system is accounted for.

Efficient Memory Management: The allocator will minimize memory fragmentation and optimize memory usage by implementing strategies such as block splitting, coalescing of free blocks, and utilizing the csbrk system call to request memory from the operating system.

Testing and Validation: The allocator will be tested using a trace-driven driver program, which simulates real-world memory allocation and deallocation operations. The program will test correctness, space utilization, and throughput by running a set of trace files that simulate various allocation and deallocation patterns.

Unit Testing Framework: A unit testing framework is provided to allow for detailed testing of individual components of the allocator. This includes testing functions like find, extend, split, and coalesce to ensure each part of the allocator behaves as expected.

## HOW TO RUN:
make unittest
./unittest -i unittests/example.txt (from project 2 directory)
