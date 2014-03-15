#include <iostream>
#include <chrono>

#include "DWThreadPool.h"

using namespace std;

static int cnt = 0;
std::mutex cnt_mutex;	// lock for cnt

void fun(int &n)
{
	std::unique_lock<std::mutex> ulock(cnt_mutex);

	cout << "[" << cnt << ", " << n << "]";
	cout << "In thread: " << std::this_thread::get_id() << endl;;
	cnt++;
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
		cout << "Next i = " << i << endl;
		pool->run(std::bind(fun, i));
	}

	// stop the pool when all task
	// has been finished
	while(pool->hasRemain()){}
	pool->stop();

	return 0;
}
