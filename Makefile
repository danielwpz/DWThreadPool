CC = g++
CFLAGS = -std=c++11
CFLAGS += -g

THREAD_SRC = test_thread.cpp
POOL_SRC = DWThreadPool.cpp

pool: $(POOL_SRC) $(THREAD_SRC)
	$(CC) $(CFLAGS) $(POOL_SRC) $(THREAD_SRC) -o pool.out -pthread

thread: $(THREAD_SRC)
	$(CC) $(CFLAGS) $(THREAD_SRC) -o thread -pthread

clear:
	-@rm *.out core
