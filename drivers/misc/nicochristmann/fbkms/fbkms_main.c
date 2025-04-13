#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include "fbkms_connector_helpers.h"
#include "fbkms_types.h"

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{

    if (!plane_state || !plane_state->fb)
      return;

    struct fbkms_device *fbkms = container_of(pipe->crtc.dev, struct fbkms_device, drm);

    struct drm_framebuffer *fb = plane_state->fb;
    struct drm_gem_cma_object *cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    void *src = cma_obj->vaddr;

    if (!fbkms->fb)
        return;

    memcpy(fbkms->fb->screen_base, src,
           fb->height * fb->pitches[0]);
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    // No-op for now
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .enable = fbkms_pipe_enable,
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
    fbkms_setup_mode(fbkms);

    ret = drm_simple_display_pipe_init(&fbkms->drm, &fbkms->pipe,
                                       &fbkms_pipe_funcs, fbkms_formats,
                                       ARRAY_SIZE(fbkms_formats),
                                       NULL,
                                       fbkms->pipe.connector);
    if (ret)
        return ret;

    drm_connector_helper_add(fbkms->pipe.connector, &fbkms_conn_helper_funcs);

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
