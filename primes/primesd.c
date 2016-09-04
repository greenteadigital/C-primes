#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "primes.h"


int main(int argc, char *argv[]) {
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in addr;
	
	memset((void *)&addr, 0, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PRIMES_DAEMON_PORT);
	inet_pton(AF_INET, "127.0.0.1", &(addr.sin_addr.s_addr));
	
	bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));
	char buffsz = 4;
	char buff[buffsz];
	
	struct sockaddr_in remote;
	socklen_t remotesz = sizeof(remote);
	
	char recv_count = 0;
	
	while(recv_count < buffsz) {
		 recv_count += recvfrom(sockfd, (void *)(&buff + recv_count), sizeof(buff) - recv_count, 0, (struct sockaddr *) &remote, &remotesz);
	}
	
	printf("Client wants a %s-bit prime\n", &buff[0]);
	printf("Client sent request from port %d\n", ntohs(remote.sin_port));
}