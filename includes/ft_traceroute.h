#ifndef FT_TRACEROUTE_H
#define FT_TRACEROUTE_H

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "options.h"

#define ERROR(msg) dprintf(2, "ft_traceroute: %s: %s\n", msg, strerror(errno));

typedef struct s_packet
{
	struct icmp		icmp;
	struct iphdr	hdr;

	char*	payload;

} t_packet;

typedef struct s_settings
{
	char*	target;
	size_t	max_hops;
	size_t	packet_size;
	size_t	nqueries;

	int		debug;

	size_t	port;

} t_settings;

typedef struct s_connection_info
{
	int sendfd;
	int	recvfd;

	t_settings	settings;
	t_packet*	packets;

	struct sockaddr_in dest;
} t_infos;

// ft_traceroute.c
int					find_path(t_infos* infos);

// packets.c
void				show_packet(unsigned char* packet, size_t size);
int					send_packet(t_infos* infos, t_packet* packet);

// socket.c
struct addrinfo*	getAddrIP(const char* name, char** ip);
int					create_socket(t_infos* infos);


// utils.c
t_settings			create_settings(t_option* options);
char*				iptoa(const char* ip);
long				gettime();



#endif // !FT_TRACEROUTE_H
