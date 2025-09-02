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

#define MAX_FD 10

int main(void)
{
	char* ip;

	struct addrinfo* info = getAddrIP("google.com", &ip);

	struct sockaddr_in	addr;
	ft_bzero(&addr, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(8008); // just a random port
	inet_pton(AF_INET, ip, &addr.sin_addr);
	printf("%s\n", ip);
	
	int socketfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (socketfd == -1)
	{
		dprintf(2, "%s", strerror(errno));
		return 1;
	}

	// int ttl = 1;
	// if (setsockopt(socketfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(size_t)) != 0)
	// {
	// 	dprintf(2, "%s", strerror(errno));
	// 	return -1;
	// }

	fd_set readfd;

	struct timeval tv;
	ft_bzero(&tv, sizeof(struct timeval));
	tv.tv_sec = 5;

	char* buffer = "hello :)";
	while (1)
	{
		FD_ZERO(&readfd);
		FD_SET(socketfd, &readfd);

		int nb_send = sendto(socketfd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
		printf("%d\n", nb_send);

		int nb = select(socketfd + 1, &readfd, NULL, NULL, &tv);
		if (nb == -1) // error
		{
			dprintf(2, "select error: %s\n", strerror(errno));
			exit(1);
		}
		if (nb == 0) // timeout
		{
			dprintf(2, "timeout\n");
			continue;
		}
		printf("got something");
	}

	

}
