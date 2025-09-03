#include "libft/memory.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>


struct addrinfo* getAddrIP(const char* name, char** ip)
{
	struct addrinfo		hint;
	struct addrinfo*	res;
	int					status;
	char				buffer[INET_ADDRSTRLEN];

	ft_bzero(&hint, sizeof(struct addrinfo));
	hint.ai_family = AF_INET;
	hint.ai_protocol = IPPROTO_UDP;
	hint.ai_socktype = SOCK_DGRAM;
	
	status = getaddrinfo(name, 0, &hint, &res);
	if (status != 0)
	{
		dprintf(2, "unknow");
		return NULL;
	}

	if (res->ai_family == AF_INET)
	{
		struct sockaddr_in* ip4 = (struct sockaddr_in*)res->ai_addr;
		inet_ntop(res->ai_family, &(ip4->sin_addr), buffer, sizeof(buffer));
		*ip = strdup(buffer);
	}

	return res;
}

#define ERROR(msg) dprintf(2, "ft_traceroute: %s: %s\n", msg, strerror(errno));

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("missing args\n");
		return 1;
	}

	char *ip;
	getAddrIP(argv[1], &ip);

	printf("ip: %s\n", ip);

	int sendfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendfd == -1)
	{
		ERROR("failed to create send socket");
		return 1;
	}

	int recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (recvfd == -1)
	{
		ERROR("failed to create recv socket");
		close(sendfd);
		return 1;
	}

	struct sockaddr_in	dest;
	ft_bzero(&dest, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(33434);
	if (inet_aton(ip, &dest.sin_addr) == 0)
	{
		ERROR("invalid address");
		close(sendfd);
		close(recvfd);
		return 1;
	}

	char send_buf[42];
	if (sendto(sendfd, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0)
	{
		ERROR("failed to send packet");
		// cleanup(...);
		return 1;
	}

	fd_set readfds;
	struct timeval timeout = {3, 0};
	FD_ZERO(&readfds);
	FD_SET(recvfd, &readfds);

	int ready = select(recvfd + 1, &readfds, NULL, NULL, &timeout);
	if (ready == -1)
	{
		ERROR("select failed");
		return 1;
	}
	if (ready == 0)
	{
		printf("timeout\n");
		return 1; // change to continue
	}
	printf("%d\n", ready);
}
