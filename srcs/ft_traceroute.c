#include "ft_traceroute.h"
#include <arpa/inet.h>

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

int probing(t_infos* infos, int *port)
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
		int retval = probing(infos, &port);
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


