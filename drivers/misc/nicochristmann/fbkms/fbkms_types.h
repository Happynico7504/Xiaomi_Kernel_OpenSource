#ifndef FBKMS_TYPES_H
#define FBKMS_TYPES_H

struct fbkms_device {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_display_mode mode;
    struct fb_info *fb;
};

#endif /* FBKMS_TYPES_H */
