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

wms_evloop_t *wms_evloop_init(const uint16_t port)
{
    wms_evloop_t *loop = (wms_evloop_t *)malloc(sizeof(*loop));
    if (loop != NULL) {
        if (wms_evloop__setup_server(loop, &port) != 0) {
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
    clients[index - 1] = clients[*nfds - 2];
    LOG_DEBUG("clients[%lu] = clients[%lu]", index - 1, *nfds - 2);
    (*nfds)--;
    return index - 1;
}

void wms_evloop_dispatch(wms_evloop_t *loop)
{
    struct pollfd *fds;
    wms_client_t *clients;
    nfds_t nfds;
    size_t fds_size = FDS_INITIAL_CAPACITY;

    fds = (struct pollfd *)malloc(sizeof(*fds) * FDS_INITIAL_CAPACITY);
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
    nfds = 1;

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
                    
                    clients[nfds - 1].fd = client_fd;

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
                    if (nread == -1) {
                        LOG_DEBUG("Failed to receive data from client fds[%lu]", i);
                        close(fds[i].fd);
                        i = pollfd_remove(fds, clients, &nfds, i);
                    }
                    else if (nread == 0) {
                        LOG_DEBUG("Client at fds[%lu] disconnected", i);
                        close(fds[i].fd);
                        i = pollfd_remove(fds, clients, &nfds, i);
                    }
                    else {
                        LOG_DEBUG("Data from fds[%lu]: %d bytes", i, nread);
                        wms_event_on_read(&clients[i - 1], data, nread);
                        // transfer data buf ownership to the event handler
                        continue;
                    }
                
                    free(data);
                }
            }
        }
    }

exit:
    // skip the server fd
    for (size_t i = 1; i < nfds; i++) {
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
    free(loop);
}

