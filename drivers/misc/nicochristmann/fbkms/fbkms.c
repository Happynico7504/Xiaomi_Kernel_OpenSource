#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "fbkms_connector_helpers.h"

struct fbkms_device {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_display_mode mode;
    struct fb_info *fb;
};

static int fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                             struct drm_crtc_state *crtc_state,
                             struct drm_plane_state *plane_state)
{
    struct fbkms_device *fbkms = container_of(pipe->crtc.dev, struct fbkms_device, drm);
    struct drm_framebuffer *fb = plane_state->fb;
    struct drm_gem_cma_object *cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    void *src = cma_obj->vaddr;

    if (!fbkms->fb)
        return -ENODEV;

    memcpy(fbkms->fb->screen_base, src,
           fb->height * fb->pitches[0]);
    return 0;
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    // No-op for now
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .enable = (void *)fbkms_pipe_enable, // Cast to silence warning on mismatched prototype
    .disable = fbkms_pipe_disable,
};

static const uint32_t fbkms_formats[] = {
    DRM_FORMAT_XRGB8888,
};

static struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM,
    .name = "fbkms",
    .desc = "Framebuffer KMS",
    .date = "20250413",
    .gem_free_object_unlocked = drm_gem_cma_free_object,
    .dumb_create = drm_gem_cma_dumb_create,
    .dumb_destroy = drm_gem_dumb_destroy,
};

static int fbkms_probe(struct platform_device *pdev)
{
    struct fbkms_device *fbkms;
    int ret;

    fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms)
        return -ENOMEM;

    platform_set_drvdata(pdev, fbkms);

    fbkms->drm.dev = &pdev->dev;
    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, &pdev->dev);
    if (ret)
        return ret;

    fbkms->fb = registered_fb[0];
    if (!fbkms->fb)
        return -ENODEV;

    drm_mode_config_init(&fbkms->drm);

    fbkms->mode.clock = 71000;
    fbkms->mode.hdisplay = 720;
    fbkms->mode.vdisplay = 1600;
    fbkms->mode.vrefresh = 60;
    fbkms->mode.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC;
    fbkms->mode.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    strcpy(fbkms->mode.name, "720x1600");

    ret = drm_simple_display_pipe_init(&fbkms->drm, &fbkms->pipe,
                                       &fbkms_pipe_funcs, fbkms_formats,
                                       ARRAY_SIZE(fbkms_formats), NULL,
                                       &fbkms->mode);
    if (ret)
        return ret;

    drm_mode_config_reset(&fbkms->drm);

    ret = drm_dev_register(&fbkms->drm, 0);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "fbkms registered successfully\n");
    return 0;
}

static int fbkms_remove(struct platform_device *pdev)
{
    struct fbkms_device *fbkms = platform_get_drvdata(pdev);
    drm_dev_unregister(&fbkms->drm);
    drm_mode_config_cleanup(&fbkms->drm);
    return 0;
}

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_probe,
    .remove = fbkms_remove,
    .driver = {
        .name = "fbkms",
    },
};

static int __init fbkms_init(void)
{
    return platform_driver_register(&fbkms_platform_driver);
}

static void __exit fbkms_exit(void)
{
    platform_driver_unregister(&fbkms_platform_driver);
}

module_init(fbkms_init);
module_exit(fbkms_exit);

MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer KMS");
MODULE_LICENSE("GPL");
