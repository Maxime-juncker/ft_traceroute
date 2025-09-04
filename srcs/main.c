#include "libft/memory.h"
#include "ft_traceroute.h"

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
		return 1;
	}
	
	if (create_socket(&infos) != 0)
		return 1;

	find_path(&infos);
}
