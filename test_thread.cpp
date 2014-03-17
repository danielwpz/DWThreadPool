#include <unistd.h>
#include <string.h>
#include <iostream>
#include <chrono>

#include "DWThreadPool.h"

using namespace std;

static int cnt = 0;
static int std_in = 0;
std::mutex cnt_mutex;	// lock for cnt
std::mutex print_mutex;	// lock for print

void uni_lock(int &n)
{
	std::unique_lock<std::mutex> ulock(cnt_mutex);

	cout << "[" << cnt << ", " << n << "]";
	cout << "In thread: " << std::this_thread::get_id() << endl;;
	cnt++;
}

#define BUF_SIZE 6

void multi_select()
{
	fd_set rd_fd;
	struct timeval tv;
	int ret;
	char buf[BUF_SIZE];
	std::thread::id thread_id = std::this_thread::get_id();


	while(1) {
		// reset fd_set
		FD_ZERO(&rd_fd);
		FD_SET(std_in, &rd_fd);
		// reset timeval
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		// reset buffer
		bzero(&buf, BUF_SIZE);

		ret = select(std_in + 1, &rd_fd, NULL, NULL, &tv);

		std::unique_lock<std::mutex> ulock(print_mutex);
		cout << "[" << thread_id <<  ", ret=" << ret << "]" << flush;

		if (ret == 0) {
			cout << "select time out.\n";
		}else if (ret < 0) {
			cout << "select error.\n";
		}else {
			if (FD_ISSET(std_in, &rd_fd)) {
				ssize_t rd_size;
				rd_size = read(std_in, &buf, BUF_SIZE - 2);
				buf[BUF_SIZE - 1] = '\0';

				cout << "read() return " << rd_size << ", ";
				printf("'%s'\n", buf);
			}
		}
	}
}

int main(int argc, char **argv)
{
	int i;
	int num_thread, run_times;

	// parse arguments
	if (argc == 1) {
		// default values
		num_thread = 0;
		run_times = 10;
	}else {
		num_thread = atoi(argv[1]);
		run_times = atoi(argv[2]);
	}

	// init the pool
	DWThreadPool *pool;
	if (num_thread && run_times)
		pool = new DWThreadPool(num_thread);
	else
		pool = new DWThreadPool();
	pool->start();

	for (i = 0; i < run_times; i++) {
		pool->run(multi_select);
	}

	// stop the pool when all task
	// has been finished
	while(pool->hasRemain()){}
	pool->stop();

	return 0;
}
