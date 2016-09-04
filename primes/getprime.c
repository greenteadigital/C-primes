#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "primes.h"


int main(int argc, char *argv[]) {
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct sockaddr_in d_addr;	// daemon sock info
	struct sockaddr_in l_addr;	// local sock info
	
	memset((void *) &d_addr, 0, sizeof(d_addr));
	memset((void *) &l_addr, 0, sizeof(l_addr));
	
	d_addr.sin_family = AF_INET;
	d_addr.sin_port = htons(PRIMES_DAEMON_PORT);
	inet_pton(AF_INET, "127.0.0.1", &(d_addr.sin_addr.s_addr));
	
	char buff[4] = "2048";
	sendto(sockfd, (void *) &buff, sizeof(buff), 0, (struct sockaddr *) &d_addr, sizeof(d_addr));
	
	socklen_t len = sizeof(l_addr);
	getsockname(sockfd, (struct sockaddr *) &l_addr, &len);
	printf("Requested prime with bitlength %s over outbound port %d\n", &buff[0], ntohs(l_addr.sin_port));
	
//	while(1) {
//		recvfrom(sockfd, (void *)&buff, sizeof(buff), 0, (struct sockaddr *) &addr, &remotesz);
//	}
}
