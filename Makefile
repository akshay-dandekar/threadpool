.PHONY: clean prepare_build_dir
# compiler
CC = $(shell echo $$CC)
ifeq ($(CC),)
CC = gcc
endif

# linker
LD = $(shell echo $$CC)
ifeq ($(LD),)
LD = gcc
endif

SOURCE_DIR = ./src
BUILD_DIR = ./build

INC_LIBCONFUSE = `pkg-config --cflags libconfuse`
LIB_LIBCONFUSE = `pkg-config --libs libconfuse`

#debug
DEBUG = -g3 -DDEBUG=0

#OPTIMIZATIONS
OPTIMIZE = -O1

WARN = -Wall

#IMX_BUILD = -DIMX_BUILD
CFLAGS = $(DEBUG) $(OPT) $(WARN) -I src/ $(IMX_BUILD)
LDFLAGS = $(DEBUG) 

OBJS_TEST_THREADPOOL = build/src/test_threadpool.o \
					   build/src/threadpool/threadpool.o \
					   build/src/linked_list/linked_list_s.o

build/test_threadpool: $(OBJS_TEST_THREADPOOL)
	$(LD) $(LDFLAGS) $(OBJS_TEST_THREADPOOL) -lpthread -o $@

build/src/test_threadpool.o: src/test_threadpool.c
	$(CC) -c $(CFLAGS) $< -o $@

build/src/threadpool/threadpool.o: src/threadpool/threadpool.c \
	src/threadpool/threadpool.h
	$(CC) -c $(CFLAGS) $< -o $@

build/src/linked_list/linked_list_s.o: \
	src/linked_list/linked_list_s.c \
	src/linked_list/linked_list_s.h 
	$(CC) -c $(CCFLAGS) $(SOURCE_DIR)/linked_list/linked_list_s.c -o $@

prepare_build_dir:
	@echo Preparing build dir
	find ./src -type d -exec mkdir -p build/{} \;
	
clean:
	find build/ -type f -name *.o -exec rm {} \;
	find build/ -maxdepth 1 -type f -exec rm {} \;
