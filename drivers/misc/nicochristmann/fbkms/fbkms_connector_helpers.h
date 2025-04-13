#ifndef FBKMS_CONNECTOR_HELPERS_H
#define FBKMS_CONNECTOR_HELPERS_H

#include <drm/drm_connector.h>
#include <drm/drm_modes.h>

// Declare externally used structures
extern const struct drm_connector_helper_funcs fbkms_conn_helper_funcs;
extern const struct drm_connector_funcs fbkms_conn_funcs;

// Inline wrapper for mode setup
static inline int fbkms_get_modes(struct drm_connector *connector,
                                  struct drm_display_mode *preferred_mode)
{
    struct drm_display_mode *mode;

    mode = drm_mode_duplicate(connector->dev, preferred_mode);
    if (!mode)
        return 0;

    drm_mode_probed_add(connector, mode);
    connector->display_info.width_mm = 68;
    connector->display_info.height_mm = 136;
    return 1;
}

// Wrapper so the .get_modes signature matches drm_connector_funcs
static int fbkms_get_modes_wrapper(struct drm_connector *connector)
{
    extern struct drm_display_mode fbkms_preferred_mode; // Defined in your modes header
    return fbkms_get_modes(connector, &fbkms_preferred_mode);
}

// Actual connector funcs
const struct drm_connector_funcs fbkms_conn_funcs = {
    .destroy = drm_connector_cleanup,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .get_modes = fbkms_get_modes_wrapper,
};

static const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes_wrapper,
};


#endif // FBKMS_CONNECTOR_HELPERS_H
