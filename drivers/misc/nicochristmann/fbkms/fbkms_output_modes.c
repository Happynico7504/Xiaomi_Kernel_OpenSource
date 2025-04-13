#include <drm/drm_modes.h>
#include "fbkms_output_modes.h"
#include "fbkms_types.h"

struct drm_display_mode fbkms_preferred_mode = {
    .clock = 71000,
    .hdisplay = 720,
    .vdisplay = 1600,
    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
    .name = "720x1600"
};

// Mode setup function used by fbkms.c
void fbkms_setup_mode(struct fbkms_device *fbkms)
{
    fbkms->mode = fbkms_preferred_mode;
}


EXPORT_SYMBOL(fbkms_preferred_mode);
EXPORT_SYMBOL(fbkms_setup_mode);
