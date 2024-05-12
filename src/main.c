#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT "8080"
#define CONNECTION_QUEUE_SIZE 10

int create_listener_socket();
void *get_in_addr(struct sockaddr *sockaddr);

int main() {
    char client_ip[INET6_ADDRSTRLEN];

    int listener_socket = create_listener_socket();
    if (listener_socket == -1) {
        fprintf(stderr, "Server error: Error during creation of the listener socket\n");
        exit(1);
    }


    if (listen(listener_socket, CONNECTION_QUEUE_SIZE) == -1) {
        perror("An error occurred while listening for a connection");
        exit(1);
    }

    printf("Server: waiting for connections...\n");

    while (true) {
        struct sockaddr_storage client_sockaddr_storage;
        socklen_t client_sockaddr_size = sizeof client_sockaddr_storage;
        struct sockaddr* client_sockaddr = (struct sockaddr*)&client_sockaddr_storage;
        
        int client_socket = accept(listener_socket, client_sockaddr, &client_sockaddr_size);

        if (client_socket == -1) {
            perror("Cannot accept connection");
            continue;
        }

        inet_ntop(client_sockaddr_storage.ss_family,
                  get_in_addr(client_sockaddr),
                  client_ip,
                  sizeof client_ip);

        printf("Server info: got new connection from %s\n", client_ip);

        // handle it

        close(client_socket);
    }

    return 0;
}

/*
** Creates a file descriptor for the new listener socket.
** Returns the created file descriptor, or -1 for errors.
*/
int create_listener_socket() {
    struct addrinfo addr_hints, *service_info;
    memset(&addr_hints, 0, sizeof addr_hints);
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_flags = AI_PASSIVE;

    int translation_result = getaddrinfo(NULL, SERVER_PORT, &addr_hints, &service_info);
    if (translation_result < 0) {
        fprintf(stderr, "Server error: Error during addrinfo translation: %s\n", gai_strerror(translation_result));
        return -1;
    }

    int listener_socket;
    struct addrinfo *addr_ptr;
    for (addr_ptr = service_info; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next) {
        listener_socket = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol);
        if (listener_socket == -1) {
            perror("Cannot create socket");
            continue;
        }

        int yes = 1;
        int setsockopt_result = setsockopt(listener_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (setsockopt_result == -1) {
            perror("Cannot set socket options");
            return -1;
        }

        if (bind(listener_socket, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
            close(listener_socket);
            perror("Cannot bind socket to addr");
            continue;
        }

        break;
    }

    freeaddrinfo(service_info);

    if (addr_ptr == NULL)  {
        fprintf(stderr, "Server error: socket could not be created. No available address was found.\n");
        return -1;
    }

    return listener_socket;
}

/*
**  Returns a reference to IPv4 or IPv6 Internet address struct
*/
void *get_in_addr(struct sockaddr *socket_addr) {
    if (socket_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)socket_addr)->sin_addr);
    }

    return &(((struct sockaddr_in6*)socket_addr)->sin6_addr);
}