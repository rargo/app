//#ifndef LWIPPOOLS_H
//#define LWIPPOOLS_H

LWIP_MALLOC_MEMPOOL_START
LWIP_MALLOC_MEMPOOL(60, 256)
LWIP_MALLOC_MEMPOOL(40, 512)
LWIP_MALLOC_MEMPOOL(20, 1512)
LWIP_MALLOC_MEMPOOL_END
 
//#endif
