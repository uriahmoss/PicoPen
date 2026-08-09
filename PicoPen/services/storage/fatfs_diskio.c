#include "ff.h"
#include "diskio.h"

#include <stddef.h>

#include "picopen/sd.h"

DSTATUS disk_initialize(BYTE drive) {
    return drive == 0u ? 0u : STA_NOINIT;
}

DSTATUS disk_status(BYTE drive) {
    return drive == 0u ? 0u : STA_NOINIT;
}

DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count) {
    if ((drive != 0u) || (buffer == NULL) || (count == 0u)) {
        return RES_PARERR;
    }
    return picopen_sd_read_blocks((uint32_t)sector, buffer, (uint32_t)count)
               ? RES_OK
               : RES_ERROR;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count) {
    (void)drive;
    (void)buffer;
    (void)sector;
    (void)count;
    return RES_WRPRT;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer) {
    (void)command;
    (void)buffer;
    return drive == 0u ? RES_PARERR : RES_NOTRDY;
}
