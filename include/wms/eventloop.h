#ifndef _WMS_EVENTLOOP_H
#define _WMS_EVENTLOOP_H

typedef struct wms_evloop_s {

} wms_evloop_t;

wms_evloop_t *wms_evloop_init();
void wms_evloop_free(wms_evloop_t *);

#endif // _WMS_EVENTLOOP_H

