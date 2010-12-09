#include <sys/stat.h>
#include <fcntl.h>
#include "mc/mc.h"
#include "private.h"
#include "xbt/log.h"
#define _GNU_SOURCE



XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_memory, mc,
                                "Logging specific to MC (memory)");

/* Pointers to each of the heap regions to use */
void *std_heap = NULL;          /* memory erased each time the MC stuff rollbacks to the beginning. Almost everything goes here */
void *raw_heap = NULL;          /* memory persistent over the MC rollbacks. Only MC stuff should go there */

/* Initialize the model-checker memory subsystem */
/* It creates the two heap regions: std_heap and raw_heap */
void MC_memory_init()
{
/* Create the first region HEAP_OFFSET bytes after the heap break address */
  std_heap = mmalloc_get_default_md();
  xbt_assert(std_heap != NULL);

/* Create the second region a page after the first one ends + safety gap */
/*  raw_heap_fd = shm_open("raw_heap", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);*/
  raw_heap = mmalloc_attach(-1, (char*)(std_heap) + STD_HEAP_SIZE + getpagesize());
  xbt_assert(raw_heap != NULL);
}

/* Finish the memory subsystem */
#include "xbt_modinter.h"
void MC_memory_exit(void)
{
  if (raw_heap)
    mmalloc_detach(raw_heap);
}
