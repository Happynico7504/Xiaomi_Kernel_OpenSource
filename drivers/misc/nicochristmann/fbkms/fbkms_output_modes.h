#ifndef FBKMS_OUTPUT_MODES_H
#define FBKMS_OUTPUT_MODES_H

#include "fbkms_types.h"

static inline void fbkms_setup_mode(struct fbkms_device *fbkms)
{
    fbkms->mode.clock = 71000;
    fbkms->mode.hdisplay = 720;
    fbkms->mode.vdisplay = 1600;
    fbkms->mode.vrefresh = 60;
    fbkms->mode.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;
    fbkms->mode.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    strcpy(fbkms->mode.name, "720x1600");
}

#endif // FBKMS_OUTPUT_MODES_H
