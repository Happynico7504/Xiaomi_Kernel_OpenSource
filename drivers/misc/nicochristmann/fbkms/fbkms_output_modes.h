#ifndef FBKMS_OUTPUT_MODES_H
#define FBKMS_OUTPUT_MODES_H

#include "fbkms_types.h"

#include <drm/drm_modes.h>

struct drm_display_mode fbkms_preferred_mode = {
    .clock = 71000,
    .hdisplay = 720,
    .vdisplay = 1600,
    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
    .name = "720x1600"
};

#endif // FBKMS_OUTPUT_MODES_H
