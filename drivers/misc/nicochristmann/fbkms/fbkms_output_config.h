struct drm_display_mode fbkms_output_config = {
    .clock = 71000,
    .hdisplay = 720,
    .vdisplay = 1600,
    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
    .name = "720x1600"
};
