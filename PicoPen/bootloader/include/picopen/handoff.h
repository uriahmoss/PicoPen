#ifndef PICOPEN_HANDOFF_H
#define PICOPEN_HANDOFF_H

#include "picopen/image_validator.h"

// Enters an already validated secure Cortex-M33 image through its vector table.
// Does not return on success.
int picopen_chain_image(const picopen_validated_image_t *image);

#endif
