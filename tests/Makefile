CC = gcc
CFLAGS = -Wall -Wextra -g

LIB_DIR = ../myThreads
LIB_SRC = $(LIB_DIR)/myThread.c
LIB_HDR = $(LIB_DIR)/myThread.h

LIB_OUT = $(LIB_DIR)/libmythread.so
TEST_OUT = test_runner

.PHONY: all run clean

all: $(LIB_OUT) $(TEST_OUT)

$(LIB_OUT): $(LIB_SRC) $(LIB_HDR)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(LIB_SRC)

$(TEST_OUT): test.c $(LIB_OUT)
	$(CC) $(CFLAGS) -I$(LIB_DIR) -L$(LIB_DIR) -o $@ test.c -lmythread -Wl,-rpath,$(LIB_DIR)
	
run: $(TEST_OUT)
	./$(TEST_OUT)

clean:
	rm -f $(LIB_OUT) $(TEST_OUT)