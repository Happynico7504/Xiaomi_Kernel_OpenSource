/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_device.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_simple_kms_helper.h>

static struct fb_info *fbinfo;

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{
    struct drm_framebuffer *fb = plane_state->fb;
    struct drm_gem_object *obj = fb->obj[0];
    struct drm_gem_fb *gem_fb = to_drm_gem_fb(fb);
    void *vaddr = gem_fb->obj[0]->vaddr;

    if (!fbinfo || !fbinfo->screen_base) {
        pr_err("fbkms: fb0 not initialized\n");
        return;
    }

    memcpy(fbinfo->screen_base, vaddr,
           fb->height * fb->pitches[0]);
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    /* No-op */
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .enable = fbkms_pipe_enable,
    .disable = fbkms_pipe_disable,
    .prepare_fb = drm_gem_fb_prepare_fb,
};

static const struct drm_mode_config_funcs fbkms_mode_config_funcs = {
    .fb_create = drm_gem_fb_create,
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};

static const struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_ATOMIC,
    .name = "fbkms",
    .desc = "DRM-KMS to FB redirect",
    .date = "20250413",
    .major = 1,
    .minor = 0,
    .patchlevel = 0,
    .fops = NULL,
    .gem_free_object_unlocked = drm_gem_free_object_unlocked,
    .dumb_create = drm_gem_cma_dumb_create,
    .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
    .prime_fd_to_handle = drm_gem_prime_fd_to_handle,
    .gem_prime_import = drm_gem_prime_import,
    .gem_prime_export = drm_gem_prime_export,
};

static int fbkms_platform_probe(struct platform_device *pdev)
{
    struct drm_device *drm;
    struct drm_simple_display_pipe *pipe;
    struct drm_display_mode *mode;
    int ret;

    fbinfo = registered_fb[0];
    if (!fbinfo) {
        dev_err(&pdev->dev, "No /dev/fb0 found\n");
        return -ENODEV;
    }

    drm = drm_dev_alloc(&fbkms_driver, &pdev->dev);
    if (IS_ERR(drm))
        return PTR_ERR(drm);

    ret = drm_dev_register(drm, 0);
    if (ret)
        return ret;

    drm_mode_config_init(drm);
    drm->mode_config.funcs = &fbkms_mode_config_funcs;

    mode = drm_mode_create(drm);
    mode->clock = 72000;
    mode->hdisplay = fbinfo->var.xres;
    mode->vdisplay = fbinfo->var.yres;
    mode->hsync_start = mode->hdisplay + 20;
    mode->hsync_end = mode->hsync_start + 20;
    mode->htotal = mode->hsync_end + 20;
    mode->vsync_start = mode->vdisplay + 10;
    mode->vsync_end = mode->vsync_start + 10;
    mode->vtotal = mode->vsync_end + 10;
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    mode->flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;
    drm_mode_set_name(mode);

    ret = drm_simple_display_pipe_init(drm, pipe, &fbkms_pipe_funcs,
                                       (const struct drm_display_mode *[]){ mode }, 1,
                                       NULL, NULL, NULL);
    if (ret)
        return ret;

    drm_mode_config_reset(drm);

    dev_info(&pdev->dev, "fbkms DRM driver initialized\n");
    return 0;
}

static const struct of_device_id fbkms_of_match[] = {
    { .compatible = "nico,fbkms" },
    { }
};
MODULE_DEVICE_TABLE(of, fbkms_of_match);

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_platform_probe,
    .driver = {
        .name = "fbkms",
        .of_match_table = fbkms_of_match,
    },
};

module_platform_driver(fbkms_platform_driver);

MODULE_AUTHOR("Nico Christmann");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Framebuffer KMS");
