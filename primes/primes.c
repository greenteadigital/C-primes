#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

int nthreads = 4;
FILE *urandom;

pthread_mutex_t printlock;
pthread_mutex_t urandom_lock;

void getUrandom(unsigned char *buff, unsigned char buffsz) {
	
	int numread = 0;
	pthread_mutex_lock(&urandom_lock);
	
	while (numread < buffsz) {
		numread += fread((void *) buff, 1, buffsz - numread, urandom);
	}
	pthread_mutex_unlock(&urandom_lock);
}

void getPrime(void *bitsize) {
	
	const char SEED_SZ_BYTES = 16;	// 128 bit random seed
	
	short PRIME_SZ = *((short*)bitsize);
	unsigned char seed_buff[SEED_SZ_BYTES];
	
	gmp_randstate_t state;
	gmp_randinit_mt(state);
	
	mpz_t seed, randnum, one;
	mpz_init(seed);
	mpz_init(randnum);
	mpz_init(one);
	
	unsigned char single = 1;
	mpz_import(one, 1, -1, 1, 0, 0, &single);
	
	getUrandom(&seed_buff[0], SEED_SZ_BYTES);
	
	mpz_import(seed, SEED_SZ_BYTES, -1, 1, 0, 0, &seed_buff[0]);
	gmp_randseed(state, seed);
	
	while(1) {
		mpz_urandomb(randnum, state, PRIME_SZ);
		mpz_ior(randnum, randnum, one);	// make sure randnum is odd by bitwise-ORing with 1
		if (mpz_probab_prime_p(randnum, 17)) break;
	}
	
	pthread_mutex_lock(&printlock);
	gmp_printf("%Zd", randnum);
	exit(0);
}

void initThreads(unsigned short bitlen) {
	
	urandom = fopen("/dev/urandom", "r");
	
	pthread_mutex_init(&printlock, NULL);
	pthread_mutex_init(&urandom_lock, NULL);
	
	pthread_t *threads[nthreads];
	
	for (int i = 0; i != nthreads; i++) {
		pthread_t new_thread;
		pthread_create(&new_thread, NULL, (void *) &getPrime, (void *) &bitlen);
		threads[i] = &new_thread;
	}
	
	for (int i = 0; i != sizeof(threads); i++) {
		pthread_join(*threads[i], NULL);
	}
	
}

int main(int argc, char *argv[]) {
	if (argc < 3) return 1;
	
	int c;
	extern char *optarg;
	extern int optind, optopt, opterr;
	unsigned short bitlen = 0;
	
	while ((c = getopt(argc, argv, "b:t:")) != -1) {
		
		switch(c) {
			
			case 'b':
				bitlen = atoi(optarg);
				break;
			
			case 't':
				nthreads = atoi(optarg);
				break;
				
			case '?':
				printf("unknown arg %c\n", optopt);
				break;
		}
	}
	
	if (bitlen > 0) initThreads(bitlen);
	return 0;
}
