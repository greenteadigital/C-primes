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

bool DBG = false;

FILE *urandom;
int sockfd; // local socket

pthread_mutex_t urandom_lock;
pthread_mutex_t served_lock;

void getUrandom(unsigned char *buff, unsigned char buffsz) {
	
	int numread = 0;
	pthread_mutex_lock(&urandom_lock);
	
	while (numread < buffsz) {
		numread += fread((void *) buff, 1, buffsz - numread, urandom);
	}
	pthread_mutex_unlock(&urandom_lock);
}

void cleanup(gmp_randstate_t *state,
			 mpz_t *seed,
			 mpz_t *randnum,
			 mpz_t *one) {
	
	mpz_clear(*seed);
	mpz_clear(*randnum);
	mpz_clear(*one);
	gmp_randclear(*state);
}

void getPrime(void *thread_params) {
	
	struct thread_params *params = (struct thread_params *) thread_params;
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
		if (params->served_prime) {
			cleanup(&state, &seed, &randnum, &one);
			params->live_thread_count -= 1;
			pthread_mutex_unlock(&served_lock);
			pthread_exit(0);
		}
		pthread_mutex_unlock(&served_lock);
		mpz_urandomb(randnum, state, params->bitsize);
		mpz_ior(randnum, randnum, one);	// make sure randnum is odd by bitwise-ORing with 1
		if (mpz_probab_prime_p(randnum, 17)) break;
	}
	
	char outstr[((int) ceil(params->bitsize / log2(16))) + 1];	// # of digits in n-bit decimal string
	memset((void *) &outstr[0], 0, sizeof(outstr));
	int send_size = gmp_sprintf((void *) &outstr[0], "%Zx", randnum);
	
	socklen_t remotesz = sizeof(params->r_addr);
	pthread_mutex_lock(&served_lock);
	
	if (!params->served_prime) {
		sendto(sockfd, (void *) &outstr, send_size, 0, (struct sockaddr *) &(params->r_addr), remotesz);
		params->served_prime = true;
		if (DBG){
			printf("\nthread params struct @ %p\n", params);
			printf("\nDaemon: sent prime over port %d: %s\n", ntohs(params->r_addr.sin_port), &outstr[0]);
		}
		cleanup(&state, &seed, &randnum, &one);
		params->live_thread_count -= 1;
		pthread_mutex_unlock(&served_lock);
		pthread_exit(0);
	} else {
		printf("\nlive_thread_count: %d\n", params->live_thread_count);
		if (params->live_thread_count == 1) {
			printf("\nfreeing thread params struct: %lu bytes\n", sizeof(*params));
			free(params);
			cleanup(&state, &seed, &randnum, &one);
			pthread_mutex_unlock(&served_lock);
			pthread_exit(0);
		}
		cleanup(&state, &seed, &randnum, &one);
		params->live_thread_count -= 1;
		pthread_mutex_unlock(&served_lock);
		pthread_exit(0);
	}
	
}

void initThreads(void *params) {
	
	for (int i = 0; i != NTHREADS; i++) {
		pthread_t new_thread;
		pthread_create(&new_thread, NULL, (void *) &getPrime, params);
		pthread_detach(new_thread);
	}
}

void usage(struct sockaddr_in *r_addr, socklen_t remotesz) {
	
	char msg[] = "Supported bit lengths are \"1024\", \"2048\", \"3072\", and \"4096\"";
	sendto(sockfd, (void *) &msg, sizeof(msg), 0, (struct sockaddr *) r_addr, remotesz);
}

int main(int argc, char *argv[]) {
	
	urandom = fopen("/dev/urandom", "r");
	
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
	
	struct thread_params params;
	char recv_count;
	
	while(1) {
		
		recv_count = 0;
		while(recv_count < buffsz) {
			recv_count += recvfrom ( sockfd,
									(void *) (&buff + (recv_count * sizeof(char))),
									sizeof(buff) - recv_count,
									0,
									(struct sockaddr *) &r_addr,
									&remotesz );
		}
		
		if ( ! ( strncmp(&buff[0], "1024", 4) == 0
			|| strncmp(&buff[0], "2048", 4) == 0
			|| strncmp(&buff[0], "3072", 4) == 0
			|| strncmp(&buff[0], "4096", 4) == 0)) {
			
			usage(&r_addr, remotesz);
			continue;
		}
		
		params.bitsize = strtol(&buff[0], 0, 0);
		params.r_addr = r_addr;
		params.served_prime = false;
		params.live_thread_count = (char) NTHREADS;
		
		void * pp = malloc(sizeof(params));
		memcpy(pp, &params, sizeof(params));
		
		if (DBG) printf("\nPreparing to send %s-bit prime to port %d\n", &buff[0], ntohs(params.r_addr.sin_port));
		
		initThreads(pp);
	}
	
}
