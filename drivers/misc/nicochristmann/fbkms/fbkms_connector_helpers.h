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

static const struct drm_connector_funcs fbkms_conn_funcs = {
    .reset = drm_atomic_helper_connector_reset,
    .fill_modes = drm_helper_probe_single_connector_modes,
    .destroy = drm_connector_cleanup,
};

static int fbkms_get_modes(struct drm_connector *connector)
{
    struct drm_display_mode *mode;

    mode = drm_mode_duplicate(connector->dev, &your_mode);
    if (!mode)
        return 0;

    drm_mode_probed_add(connector, mode);
    connector->display_info.width_mm = 68;
    connector->display_info.height_mm = 136;
    return 1;
}
