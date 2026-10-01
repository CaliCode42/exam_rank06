#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct	s_client{
	int	id;
	char	*buffer;
}		t_client;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	broadcast(int client_fd, int sockfd, int max_fd, fd_set *master_set, char *msg)
{
	for (int i = 0; i <= max_fd; i++)
	{
		if (FD_ISSET(i, master_set) && i != client_fd && i != sockfd)
		{
			send(i, msg, strlen(msg), 0);
		}
	}
}

void	clean_client(int client_fd, fd_set *master_set, t_client *client)
{
	if (FD_ISSET(client_fd, master_set))
	{
		FD_CLR(client_fd, master_set);
		close(client_fd);
	}

	if (client->buffer)
	{
		free(client->buffer);
		client->buffer = NULL;
	}
}

void	clean_all_clients(int client_fd, int sockfd, int max_fd, fd_set *master_set, t_client *cls)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (fd != sockfd && FD_ISSET(fd, master_set))
			clean_client(client_fd, master_set, &cls[client_fd]);
	}
	if (FD_ISSET(sockfd, master_set))
	{
		FD_CLR(sockfd, master_set);
		close(sockfd);
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	int sockfd, client_fd;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0)
		fatal_error();

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
		fatal_error();

	if (listen(sockfd, 10) != 0)
		fatal_error();

	fd_set		master_set, read_set;
	t_client	cls[FD_SETSIZE];

	FD_ZERO(&master_set);
	FD_SET(sockfd, &master_set);

	int	max_fd, id;

	max_fd = sockfd;
	id = 0;

	while (1)
	{
		read_set = master_set;

		if (select(max_fd + 1, &read_set, NULL, NULL, NULL) == -1)
			fatal_error();

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &read_set))
				continue ;

			if (fd == sockfd)
			{
				client_fd = accept(sockfd, NULL, NULL);
				if (client_fd < 0)
					continue ;

				cls[client_fd].id = id++;
				cls[client_fd].buffer = NULL;
				FD_SET(client_fd, &master_set);

				if (client_fd > max_fd)
					max_fd = client_fd;

				char	message[100];
				sprintf(message, "server: client %d just arrived\n", cls[client_fd].id);
				broadcast(client_fd, sockfd, max_fd, &master_set, message);
			}
			else
			{
				char	tmp[1024];
				int	bytes = 0;
				bytes = recv(fd, tmp, sizeof(tmp) - 1, 0);

				if (bytes > 0)
				{
					tmp[bytes] = '\0';
					
					char	*new_buffer = str_join(cls[fd].buffer, tmp);
					if (!new_buffer)
						fatal_error();

					cls[fd].buffer = new_buffer;

					char	*msg;
					int	status;

					while ((status = extract_message(&cls[fd].buffer, &msg)) == 1)
					{
						char	prefix[100];

						sprintf(prefix, "client %d: ", cls[fd].id);
						broadcast(fd, sockfd, max_fd, &master_set, prefix);
						broadcast(fd, sockfd, max_fd, &master_set, msg);
						free(msg);
					}
					if (status == -1)
						fatal_error();
				}
				else if (bytes == 0)
				{
					char	msg[100];

					sprintf(msg, "server: client %d just left\n", cls[fd].id);
					broadcast(fd, sockfd, max_fd, &master_set, msg);
					clean_client(fd, &master_set, &cls[fd]);
					if (fd == max_fd)
					{
						while (max_fd > sockfd && FD_ISSET(max_fd, &master_set))
							max_fd--;
					}
				}
			}
		}
	}
	close(sockfd);
	return (0);
}
