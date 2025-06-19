#ifndef FBKMS_OUTPUT_MODES_H
#define FBKMS_OUTPUT_MODES_H

#include "fbkms_types.h"

#include <drm/drm_modes.h>

void fbkms_setup_mode(struct fbkms_device *fbkms)
{
    fbkms->mode = fbkms_preferred_mode;
};

#endif // FBKMS_OUTPUT_MODES_H
