//
//  primes.h
//  Prime Number Generator
//
//  Created by Benjamin Hall on 9/3/16.
//  Copyright © 2016 Green Tea Digital. All rights reserved.
//
#include <stdbool.h>

#ifndef primes_h
#define primes_h

#define PRIMES_DAEMON_PORT 31397
#define LOCALHOST "127.0.0.1"
#define NTHREADS 4
#define SEED_SZ_BYTES 16	// * 8 = 128 bits for random seed

struct thread_params {
	struct sockaddr_in r_addr;
	short bitsize;
	volatile bool served_prime;
};

#endif /* primes_h */