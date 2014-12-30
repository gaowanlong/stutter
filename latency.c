#include <math.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef NR_MAP_PAGES
#define NR_MAP_PAGES	32768
#endif
#ifndef MSECS_DELAY
#define MSECS_DELAY	5
#endif
#define NSECS_DELAY	(MSECS_DELAY * 1000000)
#define PRINT_THRESHOLD	(5000 / MSECS_DELAY)

int exiting = 0;
void sigterm_handler(int sig)
{
	exiting = 1;
}

int main(int argc, char **argv)
{
	struct timespec intv_ts = {
		.tv_sec  = MSECS_DELAY / 1000,
		.tv_nsec =  (MSECS_DELAY % 1000) * 1000000,
	};
	struct timespec ts;
	unsigned long long time0, time1;
	unsigned long pagesize = getpagesize();
	const size_t map_size = NR_MAP_PAGES * pagesize;
	int c = 0;
	unsigned long long sum_latency = 0;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	time1 = ts.tv_sec * 1000000000LLU + ts.tv_nsec;

	signal(SIGTERM, sigterm_handler);
	while (!exiting) {
		void *map, *p;
		unsigned long latency;

		nanosleep(&intv_ts, NULL);
		map = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (map == MAP_FAILED) {
			perror("mmap");
			exit(EXIT_FAILURE);
		}

		for (p = map; p < map + map_size; p += pagesize)
			*(volatile unsigned long long *)p = time1;

		munmap(map, map_size);

		time0 = time1;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		time1 = ts.tv_sec * 1000000000LLU + ts.tv_nsec;

		latency = (time1 - time0);
		if (latency < NSECS_DELAY)
			latency = 0;
		else
			latency -= NSECS_DELAY;
		sum_latency += latency;
		if (latency < NSECS_DELAY) {
			if (++c == PRINT_THRESHOLD) {
				printf("%d %llu\n", c, (sum_latency / c));
				c = 0;
				sum_latency = 0;
			}
		} else {
			if (c)
				printf("%d %llu\n", c, (sum_latency / c));
			printf("1 %lu\n", latency);
			c = 0;
			sum_latency = 0;
		}
	}

	fflush(NULL);
	exit(EXIT_SUCCESS);
}
