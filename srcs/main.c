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
	hint.ai_socktype = SOCK_RAW;
	hint.ai_protocol = IPPROTO_ICMP;
	
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

int main(void)
{
	char* ip;

	struct addrinfo* info = getAddrIP("google.com", &ip);

	struct sockaddr_in	addr;
	ft_bzero(&addr, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ;
	addr.sin_port = 30000; // just a random port
	//
	
	int socketfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (socketfd == -1)
	{
		dprintf(2, "%s", strerror(errno));
		return 1;
	}
	

}
