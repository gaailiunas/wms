#include <wms/log.h>
#include <wms/evloop.h>
#include <wms/client.h>
#include <wms/event.h>

#include <stddef.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/poll.h>
#include <arpa/inet.h>

#define FDS_INITIAL_CAPACITY 16
#define POLL_TIMEOUT 0
#define BUF_SIZE 4096

static int wms_evloop__setup_server(wms_evloop_t *loop, const uint16_t *port)
{
    loop->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (loop->server_fd == -1) {
        LOG_DEBUG("Failed to create socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(loop->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_DEBUG("setsockopt(SO_REUSEADDR) failed");
        close(loop->server_fd);
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(*port);
    sin.sin_addr.s_addr = INADDR_ANY; // accessable to LAN
    
    if (bind(loop->server_fd, (const struct sockaddr *)&sin, sizeof(sin)) != 0) {
        LOG_DEBUG("Failed to bind");
        close(loop->server_fd);
        return 1;
    }

    if (listen(loop->server_fd, SOMAXCONN) != 0) {
        LOG_DEBUG("Failed to listen");
        close(loop->server_fd);
        return 1;
    }
    
    return 0;
}

static int wms_evloop__setup_multicast_server(wms_evloop_t *loop, const char *addr, const uint16_t *port)
{
    loop->multicast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (loop->multicast_fd == -1) {
        LOG_DEBUG("Failed to create multicast socket");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(loop->multicast_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_DEBUG("setsockopt(SO_REUSEADDR) failed");
        close(loop->multicast_fd);
        return 1;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(*port);
    sin.sin_addr.s_addr = INADDR_ANY;

    if (bind(loop->multicast_fd, (const struct sockaddr *)&sin, sizeof(sin)) != 0) {
        LOG_DEBUG("Failed to bind multicast socket");
        close(loop->multicast_fd);
        return 1;
    }

    struct ip_mreq mreq;
    if (inet_pton(AF_INET, addr, &mreq.imr_multiaddr) != 1) {
        LOG_DEBUG("Failed to turn multicast ip to ipv4");
        close(loop->multicast_fd);
        return 1;
    }
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    // join the group
    if (setsockopt(loop->multicast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        LOG_DEBUG("Failed to subscribe to multicast group");
        close(loop->multicast_fd);
        return 1;
    }

    return 0;
}

wms_evloop_t *wms_evloop_init(const uint16_t port, const char *multicast_addr,
        const uint16_t multicast_port)
{
    wms_evloop_t *loop = (wms_evloop_t *)malloc(sizeof(*loop));
    if (loop != NULL) {
        if (wms_evloop__setup_server(loop, &port) != 0) {
            free(loop);
            return NULL;
        }
        if (wms_evloop__setup_multicast_server(loop, multicast_addr, &multicast_port) != 0) {
            close(loop->server_fd);
            free(loop);
            return NULL;
        }
    }
    return loop;
}

static inline int pollfd_remove(struct pollfd *fds, wms_client_t *clients, nfds_t *nfds, size_t index)
{
    fds[index] = fds[*nfds - 1];
    LOG_DEBUG("fds[%lu] = fds[%lu]", index, *nfds - 1);
    clients[index - 2] = clients[*nfds - 3];
    LOG_DEBUG("clients[%lu] = clients[%lu]", index - 2, *nfds - 3);
    (*nfds)--;
    return index - 1;
}

void wms_evloop_dispatch(wms_evloop_t *loop)
{
    struct pollfd *fds;
    wms_client_t *clients;
    nfds_t nfds;
    size_t fds_size = FDS_INITIAL_CAPACITY;

    fds = (struct pollfd *)malloc(sizeof(*fds) * fds_size);
    if (fds == NULL) {
        LOG_DEBUG("Failed to allocate fds dynamic array");
        return;
    }

    clients = (wms_client_t *)malloc(sizeof(*clients) * fds_size - 1);
    if (clients == NULL) {
        free(fds);
        LOG_DEBUG("Failed to allocate fds dynamic array");
        return;
    }
    
    fds[0].fd = loop->server_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = loop->multicast_fd;
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    nfds = 2;

    int res;
    while ((res = poll(fds, nfds, POLL_TIMEOUT)) >= 0) {
        if (res == 0)
            continue;
        
        for (size_t i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN)  {
                LOG_DEBUG("Got data from fds[%lu]", i);
                if (i == 0) {
                    int client_fd = accept(loop->server_fd, NULL, NULL);
                    if (client_fd == -1) {
                        LOG_DEBUG("Accepted invalid client fd");
                        continue;
                    }

                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    fds[nfds].revents = 0;
                    
                    clients[nfds - 2].fd = client_fd;

                    nfds++; 

                    if (fds_size == nfds) {
                        fds_size *= 2;
                        struct pollfd *tmp0 = (struct pollfd *)realloc(fds, fds_size);
                        if (tmp0 == NULL) {
                            LOG_WARN("Memory exhaustion. Cannot increase memory of the fds array");
                            goto exit;
                        }

                        wms_client_t *tmp1 = (wms_client_t *)realloc(clients, fds_size - 1);
                        if (tmp1 == NULL) {
                            LOG_WARN("Memory exhaustion. Cannot increase memory of the clients array");
                            free(tmp0);
                            goto exit;
                        }

                        fds = tmp0;
                        clients = tmp1;
                    }
                }
                else {
                    char *data = (char *)malloc(BUF_SIZE);
                    if (data == NULL) {
                        LOG_WARN("Memory exhaustion. Failed to allocate space for data");
                        goto exit;
                    }
                    
                    int nread = recv(fds[i].fd, data, BUF_SIZE, 0);

                    if (i == 1) {
                        // multicast
                        if (nread == -1) {
                            LOG_WARN("Multicast socket broke");
                        }
                        else if (nread == 0) {
                            LOG_WARN("Disconnected from multicast socket");
                        }
                        else {
                            wms_event_on_multicast_announce(data, nread);
                            // transfer data buf ownership to the event handler
                            continue;
                        }

                        free(data);
                        goto exit;
                    }
                    else {
                        if (nread == -1) {
                            LOG_DEBUG("Failed to receive data from client fds[%lu]", i);
                        }
                        else if (nread == 0) {
                            LOG_DEBUG("Client at fds[%lu] disconnected", i);
                        }
                        else {
                            LOG_DEBUG("Data from fds[%lu]: %d bytes", i, nread);
                            wms_event_on_read(&clients[i - 2], data, nread);
                            // transfer data buf ownership to the event handler
                            continue;
                        }
                    
                        close(fds[i].fd);
                        i = pollfd_remove(fds, clients, &nfds, i);
                        free(data);
                    }
                }
            }
        }
    }

exit:
    LOG_DEBUG("Exiting event loop");

    // skip the server and multicast fds
    for (size_t i = 2; i < nfds; i++) {
        if (fds[i].fd != -1) {
            LOG_DEBUG("Closing fds[%lu], fd:%d", i, fds[i].fd);
            close(fds[i].fd);
        }
    }
    free(fds);
    free(clients);
}

void wms_evloop_free(wms_evloop_t *loop)
{
    close(loop->server_fd);
    close(loop->multicast_fd);
    free(loop);
}

