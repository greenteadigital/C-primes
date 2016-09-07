#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>

#include "primes.h"


int main(int argc, char *argv[]) {
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in r_addr;	// remote daemon sock info
	struct sockaddr_in l_addr;	// local client sock info
	
	memset((void *) &r_addr, 0, sizeof(r_addr));
	socklen_t remotesz = sizeof(r_addr);
	memset((void *) &l_addr, 0, sizeof(l_addr));
	
	r_addr.sin_family = AF_INET;
	r_addr.sin_port = htons(PRIMES_DAEMON_PORT);
	inet_pton(AF_INET, LOCALHOST, &(r_addr.sin_addr.s_addr));
	
	char buff[4] = "2048";
	sendto(sockfd, (void *) &buff, sizeof(buff), 0, (struct sockaddr *) &r_addr, sizeof(r_addr));
	
	socklen_t len = sizeof(l_addr);
	getsockname(sockfd, (struct sockaddr *) &l_addr, &len);
	
	//printf("Client: sent request for a %s-bit prime over port %d\n", &buff[0], ntohs(l_addr.sin_port));
	
	ushort prime_decimal_strlen = (int) ceil(2048 * log(2));
	char outstr[prime_decimal_strlen];
	recvfrom(sockfd, (void *) &outstr, prime_decimal_strlen, 0, (struct sockaddr *) &r_addr, &remotesz);
	printf("Client: %s\n", &outstr[0]);
	
}
