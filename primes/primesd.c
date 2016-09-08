#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <gmp.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>

#include "primes.h"

bool dbg = false;

FILE *urandom;
int sockfd; // local socket

pthread_mutex_t sendto_lock;
pthread_mutex_t urandom_lock;
pthread_mutex_t served_lock;
volatile bool served_prime = false;

void getUrandom(unsigned char *buff, unsigned char buffsz) {
	
	int numread = 0;
	pthread_mutex_lock(&urandom_lock);
	
	while (numread < buffsz) {
		numread += fread((void *) buff, 1, buffsz - numread, urandom);
	}
	pthread_mutex_unlock(&urandom_lock);
}

void getPrime(void *p_params) {
	
	const char SEED_SZ_BYTES = 16;	// 128 bit random seed
	
	struct prime_params params = *((struct prime_params *) p_params);
	short PRIME_SZ = params.bitsize;
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
		pthread_mutex_lock(&served_lock);
		if (served_prime) {
			pthread_mutex_unlock(&served_lock);
			pthread_exit(0);
		}
		pthread_mutex_unlock(&served_lock);
		mpz_urandomb(randnum, state, PRIME_SZ);
		mpz_ior(randnum, randnum, one);	// make sure randnum is odd by bitwise-ORing with 1
		if (mpz_probab_prime_p(randnum, 17)) break;
	}
	
	ushort prime_decimal_strlen = (int) ceil(PRIME_SZ / log2(10));
	char outstr[prime_decimal_strlen + 1];
	memset((void *) &outstr[0], 0, sizeof(outstr));
	int send_size = gmp_sprintf((void *) &outstr[0], "%Zd", randnum);
	
	socklen_t remotesz = sizeof(params.r_addr);
	if (dbg) printf("\nPreparing to send prime to port %d\n", ntohs(params.r_addr.sin_port));
	pthread_mutex_lock(&sendto_lock);
	pthread_mutex_lock(&served_lock);
	
	if (!served_prime) {
		if (dbg) printf("\n\nDaemon: %s", &outstr[0]);
		sendto(sockfd, (void *) &outstr, send_size, 0, (struct sockaddr *) &params.r_addr, remotesz);
		served_prime = true;
		pthread_mutex_unlock(&served_lock);
		pthread_mutex_unlock(&sendto_lock);
	} else {
		pthread_mutex_unlock(&served_lock);
		pthread_mutex_unlock(&sendto_lock);
		pthread_exit(0);
	}
	
}

void initThreads(struct prime_params *params) {
	
	for (int i = 0; i != NTHREADS; i++) {
		pthread_t new_thread;
		pthread_create(&new_thread, NULL, (void *) &getPrime, (void *) params);
	}
}

int main(int argc, char *argv[]) {
	
	urandom = fopen("/dev/urandom", "r");
	
	pthread_mutex_init(&sendto_lock, NULL);
	pthread_mutex_init(&urandom_lock, NULL);
	pthread_mutex_init(&served_lock, NULL);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in l_addr;	// local daemon sock info
	memset((void *) &l_addr, 0, sizeof(l_addr));
	
	l_addr.sin_family = AF_INET;
	l_addr.sin_port = htons(PRIMES_DAEMON_PORT);
	inet_pton(AF_INET, LOCALHOST, &(l_addr.sin_addr.s_addr));
	
	bind(sockfd, (struct sockaddr*) &l_addr, sizeof(l_addr));
	char buffsz = 4;
	char buff[buffsz];
	
	struct sockaddr_in r_addr;	// remote client sock info
	socklen_t remotesz = sizeof(r_addr);
	memset((void *) &r_addr, 0, sizeof(r_addr));
	
	while(1) {
		
		char recv_count = 0;
		while(recv_count < buffsz) {
			 recv_count += recvfrom ( sockfd,
									  (void *) (&buff + recv_count),
									  sizeof(buff) - recv_count,
									  0,
									  (struct sockaddr *) &r_addr,
									  &remotesz );
		}
		
		struct prime_params params;
		params.bitsize = strtol(&buff[0], 0, 0);
		params.r_addr = r_addr;
		
		pthread_mutex_lock(&served_lock);
		served_prime = false;
		pthread_mutex_unlock(&served_lock);
		
		initThreads(&params);
	}
	
}