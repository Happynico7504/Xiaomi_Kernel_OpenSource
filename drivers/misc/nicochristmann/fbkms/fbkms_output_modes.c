#include "fbkms_types.h"

#include <drm/drm_modes.h>
#include "fbkms_output_modes.h"

int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *preferred_mode);
int fbkms_get_modes_wrapper(struct drm_connector *connector);

void fbkms_setup_mode(struct fbkms_device *fbkms)
{
    fbkms->mode = fbkms_preferred_mode;
};
