//
//  primes.h
//  Prime Number Generator
//
//  Created by Benjamin Hall on 9/3/16.
//  Copyright © 2016 Green Tea Digital. All rights reserved.
//

#ifndef primes_h

	#define primes_h

	#define PRIMES_DAEMON_PORT 31397
	#define LOCALHOST "127.0.0.1"
	#define NTHREADS 4

struct prime_params {
	short bitsize;
	struct sockaddr_in *r_addr;
};

#endif /* primes_h */