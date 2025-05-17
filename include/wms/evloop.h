#ifndef _WMS_EVLOOP_H
#define _WMS_EVLOOP_H

#include <stdint.h>

typedef struct wms_evloop_s {
    int server_fd;
    int multicast_fd;
} wms_evloop_t;

wms_evloop_t *wms_evloop_init(const uint16_t port,
        const char *multicast_addr,
        const uint16_t multicast_port);
void wms_evloop_dispatch(wms_evloop_t *);
void wms_evloop_free(wms_evloop_t *);

#endif // _WMS_EVLOOP_H

