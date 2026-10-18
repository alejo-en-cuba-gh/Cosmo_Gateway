/*
 * memory.c
 *
 *  Created on: 28 dic 2023
 */

#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/sysbios/heaps/HeapTrack.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/IHeap.h>

#include "common.h"

void get_free_ram()
{
    char lm[70];
    Memory_Stats stats;

    // Get the heap statistics
    Memory_getStats(NULL, &stats);

    // Print memory statistics
    sprintf(lm, "Available extended RAM: %d of %d Bytes.", stats.totalFreeSize, stats.totalSize);
    logevent(L_INFO, lm);
}

Ptr mem_alloc(size_t size)
{
    Ptr m;

    m = Memory_alloc(NULL, size, 0, NULL);

    // For debug purposes
    /*if (m == NULL)
      {
        Task_sleep(100);
    }*/
    return m;
}

void mem_free(Ptr p, size_t size)
{
    if (p && size)
    {
        Memory_free(NULL, p, size);
    }
}

