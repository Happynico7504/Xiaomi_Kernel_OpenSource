struct fbkms_device {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_display_mode mode;
    struct drm_connector connector;
    struct fb_info *fb;
};
