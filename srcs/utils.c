#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#include "ft_traceroute.h"
#include "libft/memory.h"
#include "libft/string.h"

t_settings create_settings(t_option* options)
{
	t_settings settings = {
		.max_hops		= (long)get_option(options, MAX_TTL)->data,
		.nqueries		= (long)get_option(options, NQUERIES)->data,
		.packet_size	= (long)get_option(options, SIZE)->data,
		.port			= (long)get_option(options, PORT)->data,
		.debug			= (long)get_option(options, DEBUG)->data,
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


char* iptoa(const char* ip)
{
	struct sockaddr_in	sa;
	socklen_t			len;
	char				hbuf[1024]; // Max 10 result;
	ft_bzero(&sa, sizeof(struct sockaddr_in));

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


