#include "libft/memory.h"
#include "libft/string.h"
#include "libft/is.h"
#include "ft_traceroute.h"
#include "options.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <sys/time.h>

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


void show_packet(unsigned char* packet, size_t size)
{
	size_t	i;
	char	buffer[17] = {0};
	printf("+++ DUMP DATA +++\n");
	for (i = 0; i < size; i++)
	{
		printf("%c%X", packet[i] < 16 ? '0' : '\0', packet[i]);
		buffer[i % 16] = ft_isprint(packet[i]) ? packet[i] : '.';
		if (i % 16 == 15)
			printf("  %s\n", buffer);
		else if (i % 2)
			printf(" ");
	}

	while (i % 16 > 0)
	{
		printf("..");
		if (i % 2)
			printf(" ");
		buffer[i % 16] = '.';
		i++;
		if (i % 16 == 0)
			printf(" %s\n", buffer);
	}
}

t_packet process_packet(char* packet, size_t size)
{
	struct iphdr*	hdr = (struct iphdr*)packet;
	struct icmp*	icmp = (struct icmp*)(packet + sizeof(struct iphdr));
	(void)icmp;
	
	// show_packet((unsigned char*)packet, size);
	(void)size;

	return (t_packet) {
		.icmp = *icmp,
		.hdr = *hdr,
		.payload = packet + sizeof(struct iphdr) + sizeof(struct icmp)
	};
}

int send_packet(t_infos* infos, t_packet* packet)
{
	char send_buf[56]	= {0};
	char recv_buf[128]	= {0};

	if (sendto(infos->sendfd, send_buf, sizeof(send_buf), 0, (struct sockaddr*)&infos->dest, sizeof(infos->dest)) < 0)
	{
		ERROR("failed to send packet");
		// cleanup(...);
		return 1;
	}

	fd_set readfds;
	struct timeval timeout = {0, 50000};
	FD_ZERO(&readfds);
	FD_SET(infos->recvfd, &readfds);

	int ready = select(infos->recvfd + 1, &readfds, NULL, NULL, &timeout);
	if (ready == -1)
	{
		ERROR("select failed");
		return 1;
	}
	if (ready == 0)
		return 1;

	socklen_t len = sizeof(infos->dest);
	struct sockaddr* addr = NULL;
	recvfrom(infos->recvfd, recv_buf, sizeof(recv_buf), 0, addr, &len);
	*packet = process_packet(recv_buf, sizeof(recv_buf));

	return 0;
}

char* iptoa(const char* ip)
{
	struct sockaddr_in	sa;
	socklen_t			len;
	char				hbuf[1024]; // Max 10 result;
	bzero(&sa, sizeof(struct sockaddr_in));

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ip);
	len = sizeof(struct sockaddr_in);

	if (getnameinfo((struct sockaddr*)&sa, len, hbuf, sizeof(hbuf), NULL, 0, NI_DGRAM) != 0)
	{
		ERROR("could not resolve host");
		return NULL;
	}
	return ft_strdup(hbuf);
}

long gettime()
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000 + tv.tv_usec);
}

void print_res(t_infos* infos, int idx, float time)
{
	struct in_addr tmp;

	tmp.s_addr = infos->packets[idx].hdr.saddr;
	char*	ip = inet_ntoa(tmp);

	int i = 0;
	for ( ; i < idx; i++)
	{
		if (infos->packets[i].hdr.saddr == infos->packets[idx].hdr.saddr)
			break;
	}
	if (i == idx) // packet is new
		printf("%s (%s) ", iptoa(ip), ip);
	printf("%.3fms ", time);
}

int do_probe(t_infos* infos, int *port)
{
	int retval = 0;
	int before, after;
	float time;

	for (size_t i = 0; i < infos->settings.nqueries; i++)
	{
		before = gettime();

		infos->dest.sin_port = htons(*port);
		if (send_packet(infos, &infos->packets[i]) != 0)
		{
			printf("* ");
			continue;
		}
		after = gettime();
		time = ((float)after - before) / 1000;

		print_res(infos, i, time);
		*port += 1;
		if (infos->packets[i].icmp.icmp_type != ICMP_TIME_EXCEEDED)
			retval = 2;
	}
	return retval;
}

int find_path(t_infos* infos)
{
	size_t	ttl = 1;
	int		stop = 0;
	int port = infos->settings.port;
	while ((stop == 0 || ttl == 1)
				&& ttl <= infos->settings.max_hops)	
	{
		printf(" %ld  ", ttl);

		setsockopt(infos->sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
		int retval = do_probe(infos, &port);
		if (retval == 1)
		{
			ttl++;
			port++;
			continue;
		}
		if (retval == 2)
			stop = 1;
		printf("\n");
		ttl++;
	}
	return 0;
}

t_settings create_settings(t_option* options)
{
	t_settings settings = {
		.max_hops		= (long)get_option(options, MAX_TTL)->data,
		.nqueries		= (long)get_option(options, NQUERIES)->data,
		.packet_size	= (long)get_option(options, SIZE)->data,
		.port			= (long)get_option(options, PORT)->data,
		.target			= get_option(options, NAME)->data,
	};
	free(options);

	if (settings.nqueries > 10)
	{
		dprintf(2, "no more than 10 probes per hop\n");
		exit(1);
	}

	return settings;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("missing args\n");
		return 1;
	}

	t_infos infos;
	infos.settings = create_settings(parse_options(argc, argv));	
	infos.packets = ft_calloc(infos.settings.nqueries, sizeof(t_packet));
	if (!infos.packets)
	{
		exit(1);
	}
	

	char *ip;
	getAddrIP(infos.settings.target, &ip);

	printf("traceroute to %s, %ld hops max, %ld byte packets\n",
		ip, infos.settings.max_hops, infos.settings.packet_size);

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

	int ttl = 1;
	if (setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int)) != 0)
	{
		ERROR("failed to set ttl");
		return 1;
	}

	struct sockaddr_in	dest;
	ft_bzero(&dest, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, &dest.sin_addr) == 0) // TODO: aton forbidden
	{
		ERROR("invalid address");
		close(sendfd);
		close(recvfd);
		return 1;
	}

	infos.recvfd = recvfd;
	infos.sendfd = sendfd;
	infos.dest = dest;
	find_path(&infos);

}
