#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include <drm/drm_atomic_helper.h>
#include "fbkms_connector_helpers.h"
#include "fbkms_output_modes.h"

int fbkms_get_modes(struct drm_connector *connector, struct drm_display_mode *preferred_mode)
{
    pr_info("loading modes\n");

    struct drm_display_mode *mode = drm_mode_duplicate(connector->dev, preferred_mode);
    if (!mode)
        return 0;

    mode->type |= DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    return 1;
}

int fbkms_get_modes_wrapper(struct drm_connector *connector)
{
    pr_info("prepare for mode loading\n");

    extern struct drm_display_mode fbkms_preferred_mode;
    return fbkms_get_modes(connector, &fbkms_preferred_mode);
}
