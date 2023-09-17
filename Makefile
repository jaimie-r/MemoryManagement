# Makefile
CC = gcc
DEBUG_FLAG = -O0
DEPLOY_FLAG = -O2
OPT_FLAG = $(DEPLOY_FLAG) # -O0 for use with GDB, -O2 for testing performance and is the default setting
CFLAGS = -Wall $(OPT_FLAG) -Werror -ggdb

all: runner performance gprof_performance unittest
support.o: support.c support.h
# csbrk.o: csbrk.c csbrk.h
err_handler.o: err_handler.c err_handler.h 
# csbrk_tracked.o: csbrk.c csbrk.h
# 	$(CC) $(CFLAGS) -DTRACK_CSBRK -o csbrk_tracked.o -c csbrk.c
umalloc.o: umalloc.c umalloc.h
check_heap.o: umalloc.c umalloc.h
unittest.o: unittest.c

deploy: OPT_FLAG=$(DEPLOY_FLAG)
deploy: clean all

debug: OPT_FLAG=$(DEBUG_FLAG)
debug: clean all

runner: runner.c csbrk_tracked.o umalloc.o check_heap.o err_handler.o support.o
	$(CC) $(CFLAGS) -o runner runner.c  umalloc.h csbrk_tracked.o umalloc.o check_heap.o err_handler.o support.o

performance: performance.c csbrk.o umalloc.o support.o err_handler.o
	$(CC) $(CFLAGS) -o performance performance.c umalloc.h csbrk.o umalloc.o err_handler.o support.o

unittest: unittest.o support.o umalloc.o csbrk.o err_handler.o check_heap.o
	$(CC) $(CFLAGS) -o unittest unittest.c umalloc.h umalloc.o support.o csbrk.o err_handler.o check_heap.o


# GPROF
# gprof_csbrk.o: csbrk.c csbrk.h
# 	$(CC) -O0 -c -fprofile-arcs -g -pg -o gprof_csbrk.o csbrk.c 

gprof_umalloc.o: umalloc.c umalloc.h
	$(CC) -O0 -c -fprofile-arcs -g -pg -o gprof_umalloc.o umalloc.c	

gprof_performance: performance.c gprof_umalloc.o support.o gprof_csbrk.o
	$(CC) -O0 -fprofile-arcs -g -pg -o gprof_performance performance.c umalloc.h gprof_umalloc.o gprof_csbrk.o err_handler.o support.o

clean:
	rm -f *.so runner gprof_performance performance *.gcda gmon.out unittest \
		support.o err_handler.o umalloc.o check_heap.o unittest.o gprof_umalloc.o 