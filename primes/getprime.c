#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define DBG false

#include "primes.h"


int main(int argc, char *argv[]) {
	
	if (argc < 2) return 1;
	
	int c;
	extern char *optarg;
	extern int optind, optopt, opterr;
	char bitlen[5] = {0, 0, 0, 0, 0};
	
	while ((c = getopt(argc, argv, "b:")) != -1) {
		
		switch(c) {
				
			case 'b':
				memcpy(&bitlen, optarg, 4);
				break;
				
			case '?':
				printf("\nUnknown arg '%c'\n", optopt);
				break;
		}
	}
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in r_addr;	// remote daemon sock info
	struct sockaddr_in l_addr;	// local client sock info
	
	memset((void *) &r_addr, 0, sizeof(r_addr));
	socklen_t remotesz = sizeof(r_addr);
	memset((void *) &l_addr, 0, sizeof(l_addr));
	
	r_addr.sin_family = AF_INET;
	r_addr.sin_port = htons(PRIMES_DAEMON_PORT);
	inet_pton(AF_INET, LOCALHOST, &(r_addr.sin_addr.s_addr));
	
	sendto(sockfd, (void *) &bitlen, sizeof(bitlen), 0, (struct sockaddr *) &r_addr, sizeof(r_addr));
	
	socklen_t len = sizeof(l_addr);
	getsockname(sockfd, (struct sockaddr *) &l_addr, &len);
	
	if (DBG) printf("\nClient: sent request for a %s-bit prime over port %d\n", &bitlen[0], ntohs(l_addr.sin_port));
	
	ushort prime_hex_strlen = (int) ceil(atoi(bitlen) / 4);
	char outstr[prime_hex_strlen];
	memset(&outstr[0], 0, sizeof(outstr));
	recvfrom(sockfd, (void *) &outstr, prime_hex_strlen, 0, (struct sockaddr *) &r_addr, &remotesz);
	
	if (DBG) printf("\nClient: %s\n", &outstr[0]);
	
}
