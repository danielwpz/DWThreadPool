#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <chrono>
#include <sys/epoll.h>

#include "DWThreadPool.h"

using namespace std;

static int cnt = 0;
static int std_in = 0;
std::mutex cnt_mutex;	// lock for cnt
std::mutex print_mutex;	// lock for print

const int MAX_EVENTS = 100;
int g_epollfd;
struct epoll_event events[MAX_EVENTS];

/**
 * Thread-safe log.
 * para:
 * text - the content to be logged.
 * verbose - a flag indicates whether details
 *			 should be printed.
 */
inline void slog(std::string text, bool verbose = true)
{
	{
	std::unique_lock<std::mutex> ulock(cnt_mutex);

	if (verbose) {
		cout << "[" << std::this_thread::get_id()  << "]";
	}
	cout << text;

	ulock.unlock();
	}
}

inline void thread_sleep(unsigned int interval)
{
	std::chrono::milliseconds dura(interval);
	std::this_thread::sleep_for(dura);
}

void uni_lock()
{
	std::unique_lock<std::mutex> ulock(cnt_mutex);

	cout << "[" << cnt << "]";
	cout << "In thread: " << std::this_thread::get_id() << endl;;
	cnt++;

	ulock.unlock();
	const unsigned int work_dura = 5000;
	std::chrono::milliseconds dura(work_dura);
	std::this_thread::sleep_for(dura);
}

void multi_select()
{
	fd_set rd_fd;
	struct timeval tv;
	int ret;
	std::thread::id thread_id = std::this_thread::get_id();


	while(1) {
		// reset fd_set
		FD_ZERO(&rd_fd);
		FD_SET(std_in, &rd_fd);
		// reset timeval
		tv.tv_sec = 15;
		tv.tv_usec = 0;

		std::unique_lock<std::mutex> ulock(print_mutex);
		ret = select(std_in + 1, &rd_fd, NULL, NULL, &tv);
		cout << "[" << thread_id <<  ", ret=" << ret << "]" << flush;

		if (ret == 0) {
			cout << "select time out." << endl;
		}else if (ret < 0) {
			cout << "select error." << endl;
		}else {
			if (FD_ISSET(std_in, &rd_fd)) {
				ssize_t rd_size;
				char ch;

				/*
				 * stdin is a stream, thus the read()
				 * won't return 0 because it could al-
				 * ways expects the user to type some
				 * characters into stdin.
				 */
				while (rd_size = read(std_in, &ch, 1)) {
					cout << ch;
					if (ch == '\n')
						break;
				}

				/*
				 * Before we 'trap into' the time-
				 * consuming actual work, release
				 * the uni-lock. So that other
				 * threads could acquire the lock
				 * and do select().
				 */
				ulock.unlock();
				// working...
				const unsigned int work_dura = 5000;
				std::chrono::milliseconds dura(work_dura);
				std::this_thread::sleep_for(dura);
			}
		}
	}
}

void multi_epoll()
{
	int i;
	
	while(1) {
		int nfds = epoll_wait(g_epollfd, events, MAX_EVENTS, 10 * 1000);
		if (nfds < 0) {
			cout << "ERR: epoll_wait() failed.\n";
			continue;
		}else if (nfds == 0) {
			slog("epoll_wait() timeout.\n");	
			continue;
		}

		std::string str = "epoll_wait() returns. ";
		slog(str);

		/**
		 * Since the mode is set to be edge-triggerd,
		 * one particular thread must read all avail-
		 * able fds given by epoll.
		 */
		for (i = 0; i < nfds; i++) {
			char c;
			while(read(0, &c, 1) >= 0) {
				cout << c;
			}
			if (errno == EAGAIN)
				slog("read EAGAIN\n", false);
			slog("read over.\n", false);

			// working...
			slog("sleep...\n");
			thread_sleep(15000);
			slog("awake\n");
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

	// set stdin as non-block
	int flags = fcntl(std_in, F_GETFL, 0);
	fcntl(std_in, F_SETFL, flags | O_NONBLOCK);

	// init the epoll envoriment
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;

	g_epollfd = epoll_create(MAX_EVENTS);
	if (g_epollfd <= 0)
		cout << "ERR: epoll_create() failed.\n";

	if (epoll_ctl(g_epollfd, EPOLL_CTL_ADD, std_in, &ev) < 0)
		cout << "ERR: epoll_ctl() failed.\n";

	// init the thread pool
	DWThreadPool *pool;
	if (num_thread && run_times)
		pool = new DWThreadPool(num_thread);
	else
		pool = new DWThreadPool();
	pool->start();

	for (i = 0; i < run_times; i++) {
		pool->run(multi_epoll);
	}

	// stop the pool when all task
	// has been finished
	while(pool->hasRemain()){}
	pool->stop();
	delete pool;

	return 0;
}
