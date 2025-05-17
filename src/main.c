#include <wms/log.h>
#include <wms/evloop.h>
#include <stddef.h>

int main(void)
{
    wms_evloop_t *loop = wms_evloop_init(6969, "239.255.0.1", 12345);
    if (loop == NULL) {
        LOG_ERROR("Failed to initialize the event loop");
        return 1;
    }
    wms_evloop_dispatch(loop);
    wms_evloop_free(loop);
    return 0;
}

