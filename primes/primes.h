#pragma once

#include <stdbool.h>

#define PRIMES_DAEMON_PORT 31397
#define LOCALHOST "127.0.0.1"
#define NTHREADS 4
#define SEED_SZ_BYTES 16	// 16 	* 8 = 128 bits for random seed

struct thread_params {
	struct sockaddr_in r_addr;
	volatile bool served_prime;
	short bitsize;
	volatile char live_thread_count;
};
