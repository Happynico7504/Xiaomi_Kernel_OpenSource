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

static int fbkms_get_modes_wrapper(struct drm_connector *connector)
{
    struct fbkms_device *fbkms = container_of(connector->dev, struct fbkms_device, drm);
    return fbkms_get_modes(connector, &fbkms->mode);
}

static const struct drm_connector_funcs fbkms_conn_funcs = {
    .reset = drm_atomic_helper_connector_reset,
    .destroy = drm_connector_cleanup,
    .fill_modes = drm_helper_probe_single_connector_modes,
};

static const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes_wrapper,
};

ret = drm_simple_display_pipe_init(&fbkms->drm, &fbkms->pipe,
                                   &fbkms_pipe_funcs, fbkms_formats,
                                   ARRAY_SIZE(fbkms_formats), NULL,
                                   &fbkms->mode);
if (ret)
    return ret;
