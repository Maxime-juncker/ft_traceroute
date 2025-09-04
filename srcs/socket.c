#include "ft_traceroute.h"
#include "libft/memory.h"

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

struct addrinfo* getAddrIP(const char* name, char** ip)
{
	struct addrinfo		hint;
	struct addrinfo*	res;
	int					status;

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
		*ip = inet_ntoa(ip4->sin_addr);
	}

	return res;
}

int set_socket(t_infos* infos, char* ip)
{
	struct sockaddr_in	dest;

	ft_bzero(&dest, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &dest.sin_addr) == 0)
	{
		ERROR("invalid address");
		close(infos->sendfd);
		close(infos->recvfd);
		return 1;
	}

	infos->dest = dest;

	return 0;
}

int create_socket(t_infos* infos)
{
	char *ip;
	getAddrIP(infos->settings.target, &ip);

	printf("traceroute to %s, %ld hops max, %ld byte packets\n",
		ip, infos->settings.max_hops, infos->settings.packet_size);

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
	infos->sendfd = sendfd;
	infos->recvfd = recvfd;

	return set_socket(infos, ip);
}

