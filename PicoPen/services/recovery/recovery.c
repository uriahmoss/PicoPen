#include "picopen/recovery.h"
#include <stddef.h>
#include "picopen/boot_format.h"
#include "picopen/internal_fs.h"
#define CRASH_FILE "/crash.v1"
static picopen_crash_record_t current;
static bool available;
static uint32_t sum(const picopen_crash_record_t *record) {
    const uint8_t *bytes=(const uint8_t *)record; uint32_t value=UINT32_C(0xC4A55A3C);
    for(size_t i=0u;i<offsetof(picopen_crash_record_t,checksum);++i) value=(value<<5u)^value^bytes[i];
    return value;
}
static void load(void) {
    size_t length=0u; picopen_crash_record_t record;
    available=picopen_internal_fs_read(CRASH_FILE,&record,sizeof(record),&length) &&
        length==sizeof(record) && record.version==1u && record.size==sizeof(record) && record.checksum==sum(&record);
    if(available) current=record;
}
void picopen_recovery_init(bool watchdog_reset,uint32_t boot_scratch) {
    current=(picopen_crash_record_t){.version=1u,.size=sizeof(current)}; load();
    if(!watchdog_reset || boot_scratch!=PICOPEN_BOOT_ATTEMPT_OS_ENTERED) return;
    if(!available) current=(picopen_crash_record_t){.version=1u,.size=sizeof(current)};
    ++current.count; current.reason=boot_scratch; current.checksum=sum(&current);
    available=picopen_internal_fs_replace(CRASH_FILE,&current,sizeof(current));
}
bool picopen_recovery_get(picopen_crash_record_t *record) { if(!available||!record)return false; *record=current; return true; }
bool picopen_recovery_clear(bool locally_confirmed) { if(!locally_confirmed)return false; available=false; current=(picopen_crash_record_t){0}; return picopen_internal_fs_remove(CRASH_FILE); }
