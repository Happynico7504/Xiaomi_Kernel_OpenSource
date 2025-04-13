#ifndef FBKMS_TYPES_H
#define FBKMS_TYPES_H

#include <drm/drm_device.h>
#include <drm/drm_simple_kms_helper.h>
#include <linux/fb.h>

struct fbkms_device {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_display_mode mode;
    struct fb_info *fb;
};

#endif /* FBKMS_TYPES_H */
