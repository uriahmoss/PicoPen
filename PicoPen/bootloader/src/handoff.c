#include "picopen/handoff.h"

#include <stdint.h>

#include "hardware/irq.h"
#include "hardware/platform_defs.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"

static void __attribute__((naked, noreturn)) enter_image(
    uint32_t stack_pointer __attribute__((unused)),
    uint32_t reset_handler __attribute__((unused))) {
    __asm volatile(
        "msr msp, r0\n"
        "movs r2, #0\n"
        "msr msplim, r2\n"
        "dsb\n"
        "isb\n"
        "bx r1\n");
}

int picopen_chain_image(const picopen_validated_image_t *image) {
    (void)save_and_disable_interrupts();
    systick_hw->csr = 0u;
    systick_hw->cvr = 0u;
    for (uint interrupt = 0u; interrupt < NUM_IRQS; ++interrupt) {
        irq_set_enabled(interrupt, false);
        irq_clear(interrupt);
    }

    scb_hw->vtor = image->vector_address;
    __dsb();
    __isb();

    const uint32_t stack_pointer =
        *(const uint32_t *)(uintptr_t)image->vector_address;
    enter_image(stack_pointer, image->entry_address);
    __builtin_unreachable();
}
