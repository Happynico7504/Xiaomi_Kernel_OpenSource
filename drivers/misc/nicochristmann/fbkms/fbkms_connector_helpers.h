#ifndef FBKMS_CONNECTOR_HELPERS_H
#define FBKMS_CONNECTOR_HELPERS_H

#include <drm/drm_connector.h>
#include <drm/drm_modes.h>

static inline int fbkms_get_modes(struct drm_connector *connector,
                                  struct drm_display_mode *preferred_mode)
{

    pr_info("loading modes\n");

    struct drm_display_mode *mode = drm_mode_duplicate(connector->dev, preferred_mode);
    if (!mode)
        return 0;

    mode->type |= DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    return 1;
}

static int fbkms_get_modes_wrapper(struct drm_connector *connector)
{
    pr_info("prepare for mode loading\n");
  
    extern struct drm_display_mode fbkms_preferred_mode;
    return fbkms_get_modes(connector, &fbkms_preferred_mode);
}

static const struct drm_connector_funcs fbkms_conn_funcs = {
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = drm_connector_cleanup,
};


static const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes_wrapper,
};

static const struct drm_connector_funcs fbkms_connector_funcs = {
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = drm_connector_cleanup,

};

#endif // FBKMS_CONNECTOR_HELPERS_H
