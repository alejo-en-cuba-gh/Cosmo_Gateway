/*
 * flash.c
 *
 *  Created on: 20 dic 2023
 */

#include "common.h"
#include "config-db.h"
#include <ti/drivers/NVS.h>

NVS_Handle nvsHandle;
NVS_Attrs regionAttrs;
extern Semaphore_Handle sem_db_rw;

bool flash_init()
{
    NVS_Params nvsParams;

    NVS_Params_init(&nvsParams);
    nvsHandle = NVS_open(CONFIG_NVS_0, &nvsParams);
    if (nvsHandle == NULL)
    {
        return false;
    }
    NVS_getAttrs(nvsHandle, &regionAttrs);
    return true;
}

int_fast16_t flash_write_val(uint32_t offset, void *data, uint8_t size)
{
    int_fast16_t r;
    void *fc;

    // Get semaphore for writing to DB
    Semaphore_pend(sem_db_rw, BIOS_WAIT_FOREVER);

    // Copy all contents of region to memory
    fc = mem_alloc(regionAttrs.regionSize);
    if (fc == NULL)
    {
        return -1;
    }

    r = NVS_read(nvsHandle, 0, fc, regionAttrs.regionSize);
    if (r != NVS_STATUS_SUCCESS)
    {
        mem_free(fc, regionAttrs.regionSize);
        return r;
    }

    // Change value in RAM copy
    memcpy(fc + offset, data, size);

    // Write entire region back to flash
    NVS_erase(nvsHandle, 0, regionAttrs.regionSize);
    r = NVS_write(nvsHandle, 0, fc, regionAttrs.regionSize, NVS_WRITE_POST_VERIFY);

    // Release semaphore for writing to DB
    Semaphore_post(sem_db_rw);

    mem_free(fc, regionAttrs.regionSize);
    return r;
}

int_fast16_t flash_read_val(uint32_t offset, void *data, uint8_t size)
{
    int_fast16_t r;

    // Get semaphore for reading from from DB
    sem_db_start_read();

    r = NVS_read(nvsHandle, offset, data, size);

    // Release semaphore for reading from DB
    sem_db_end_read();
    return r;
}
