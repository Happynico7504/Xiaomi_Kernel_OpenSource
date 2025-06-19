#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include <drm/drm_atomic_helper.h>
#include "fbkms_connector_helpers.h"
#include "fbkms_output_modes.h" // falls du dort preferred_mode hast

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

const struct drm_connector_funcs fbkms_conn_funcs = {
    .reset = drm_atomic_helper_connector_reset,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = drm_connector_cleanup,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes_wrapper,
};
