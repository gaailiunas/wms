#include <wms/log.h>
#include <wms/event.h>
#include <stdlib.h>

void wms_event_on_read(wms_client_t *client, char *data, int nread)
{
    LOG_DEBUG("on_read event! fd:%d", client->fd);
    free(data);
}


void wms_event_on_multicast_announce(char *data, int nread)
{
    LOG_DEBUG("multicast announce event!");
    free(data);
}
