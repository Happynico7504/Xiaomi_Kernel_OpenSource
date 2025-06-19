#include "fbkms_types.h"

#include <drm/drm_modes.h>
#include "fbkms_output_modes.h"

int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *preferred_mode);
int fbkms_get_modes_wrapper(struct drm_connector *connector);
