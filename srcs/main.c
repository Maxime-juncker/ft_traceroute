#include "libft/memory.h"
#include "libft/is.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define ERROR(msg) dprintf(2, "ft_traceroute: %s: %s\n", msg, strerror(errno));

typedef struct s_packet
{
	struct icmp		icmp;
	struct iphdr	hdr;

	char*	payload;

} t_packet;

typedef struct s_connection_info
{
	int sendfd;
	int	recvfd;

	struct sockaddr_in dest;
} t_infos;

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
	
	show_packet((unsigned char*)packet, size);

    // struct in_addr source, dest;

    // source.s_addr = hdr->saddr;
    // dest.s_addr = hdr->daddr;

    // printf("Source IP: %s\n", inet_ntoa(source));
    // printf("Destination IP: %s\n", inet_ntoa(dest));
    //
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
	struct timeval timeout = {3, 0};
	FD_ZERO(&readfds);
	FD_SET(infos->recvfd, &readfds);

	int ready = select(infos->recvfd + 1, &readfds, NULL, NULL, &timeout);
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

	socklen_t len = sizeof(infos->dest);
	recvfrom(infos->recvfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&infos->dest, &len);
	*packet = process_packet(recv_buf, sizeof(recv_buf));

	return 0;
}

int find_path(t_infos* infos)
{
	t_packet packet;
	ft_bzero(&packet, sizeof(packet));

	int i = 0;
	int ttl = 1;
		infos->dest.sin_port = htons(33435 + i);
	while (packet.icmp.icmp_type == ICMP_TIME_EXCEEDED || i == 0)	
	{

		if (send_packet(infos, &packet) != 0)
		{
			exit(0);
		}

		struct in_addr tmp;
		tmp.s_addr = packet.hdr.daddr;
		printf("%d: %s (ttl:%d)\n", i, inet_ntoa(tmp), packet.icmp.icmp_type);
		ttl++;
		i++;
		sleep(1);
	}
	return 0;
}


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

	int ttl = 1;
	if (setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int)) != 0)
	{
		ERROR("failed to set ttl");
		return 1;
	}

	struct sockaddr_in	dest;
	ft_bzero(&dest, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	if (inet_aton(ip, &dest.sin_addr) == 0)
	{
		ERROR("invalid address");
		close(sendfd);
		close(recvfd);
		return 1;
	}

	t_infos infos;
	infos.recvfd = recvfd;
	infos.sendfd = sendfd;
	infos.dest = dest;
	find_path(&infos);

}
