#ifndef _WMS_EVENT_H
#define _WMS_EVENT_H

#include <wms/client.h>

void wms_event_on_read(wms_client_t *, char *data, int nread);
void wms_event_on_multicast_announce(char *data, int nread);

#endif // _WMS_EVENT_H

