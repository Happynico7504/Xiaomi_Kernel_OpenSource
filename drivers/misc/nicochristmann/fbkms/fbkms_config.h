struct drm_display_mode fbkms_output_config = {
    .clock = 71000,

    .hdisplay = 720,
    .hsync_start = 728,
    .hsync_end = 736,
    .htotal = 744,

    .vdisplay = 1600,
    .vsync_start = 1608,
    .vsync_end = 1616,
    .vtotal = 1624,

    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
    .name = "720x1600"
};
