#include "ft_traceroute.h"
#include "libft/is.h"

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

t_packet process_packet(char* packet)
{
	struct iphdr*	hdr = (struct iphdr*)packet;
	struct icmp*	icmp = (struct icmp*)(packet + sizeof(struct iphdr));
	
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
	*packet = process_packet(recv_buf);

	if (infos->settings.debug)
		show_packet((unsigned char*)recv_buf, sizeof(recv_buf));

	return 0;
}


