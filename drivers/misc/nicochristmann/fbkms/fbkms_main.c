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

#include <drm/drm_modes.h>
#include "fbkms_output_modes.h"

struct drm_display_mode fbkms_preferred_mode = {
    .clock = 71000,
    .hdisplay = 720,
    .vdisplay = 1600,
    .vrefresh = 60,
    .flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
    .name = "720x1600"
};

void fbkms_setup_mode(struct fbkms_device *fbkms)
{
    fbkms->mode = fbkms_preferred_mode;
}

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{
  
    struct fbkms_device *fbkms = container_of(pipe->crtc.dev, struct fbkms_device, drm);

    struct drm_framebuffer *fb = plane_state->fb;

    struct drm_gem_cma_object *cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    void *src = cma_obj->vaddr;

    memcpy(fbkms->fb->screen_base, src,
           fb->height * fb->pitches[0]);

    pr_info("fbkms pipe enabled\n");
  
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    pr_info("fbkms pipe disabled\n");
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

    pr_info("fbkms: probe started\n");

    fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms) {
        dev_err(&pdev->dev, "fbkms: failed to allocate device struct\n");
        return -ENOMEM;
    }

    fbkms->fb = registered_fb[0];
    if (!fbkms->fb || !fbkms->fb->screen_base) {
        dev_err(&pdev->dev, "fbkms: fb0 not available or screen_base null\n");
        return -ENODEV;
    }

    platform_set_drvdata(pdev, fbkms);
    pr_info("fbkms: platform data set\n");

    fbkms->drm.dev = &pdev->dev;
    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, &pdev->dev);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_init failed (%d)\n", ret);
        return ret;
    }
    pr_info("fbkms: drm_dev_init successful\n");

    if (!fbkms->drm.dev) {
      pr_err("fbkms: drm.dev is NULL!\n");
      return -EINVAL;
    }

    pr_info("fbkms: drm.dev = %px\n", &fbkms->drm);
    pr_info("fbkms: drm.dev.dev = %px\n", fbkms->drm.dev);

    drm_mode_config_init(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_init done\n");

    fbkms_setup_mode(fbkms);
    pr_info("fbkms: mode setup done\n");

    ret = drm_simple_display_pipe_init(&fbkms->drm, &fbkms->pipe,
                                       &fbkms_pipe_funcs, fbkms_formats,
                                       ARRAY_SIZE(fbkms_formats),
                                       NULL,
                                       &fbkms->pipe.connector);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_simple_display_pipe_init failed (%d)\n", ret);
        goto err_config;
    }
    pr_info("fbkms: display pipe init done\n");

    struct drm_connector *conn = &fbkms->pipe.connector;
        conn->display_info.width_mm = 68;
        conn->display_info.height_mm = 122;
        conn->polled = DRM_CONNECTOR_POLL_HPD;

    drm_mode_config_reset(&fbkms->drm);
    pr_info("fbkms: mode config reset\n");

    ret = drm_dev_register(&fbkms->drm, 0);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_register failed (%d)\n", ret);
        goto err_pipe;
    }
    pr_info("fbkms: drm_dev_register success\n");

    drm_kms_helper_poll_init(&fbkms->drm);
    pr_info("fbkms: KMS poll init done\n");

    dev_info(&pdev->dev, "fbkms driver registered successfully\n");
    return 0;

err_pipe:
    drm_mode_config_cleanup(&fbkms->drm);
err_config:
    drm_dev_put(&fbkms->drm);
    return ret;
}

static int fbkms_remove(struct platform_device *pdev)
{
    struct fbkms_device *fbkms = platform_get_drvdata(pdev);
    pr_info("fbkms: remove called\n");

    drm_kms_helper_poll_fini(&fbkms->drm);
    pr_info("fbkms: shutting down polling\n");

    drm_dev_unregister(&fbkms->drm);
    pr_info("fbkms: drm_dev_unregister done\n");

    drm_mode_config_cleanup(&fbkms->drm);
    pr_info("fbkms: drm_mode_config_cleanup done\n");

    drm_dev_put(&fbkms->drm);
    pr_info("fbkms: drm_dev_put done\n");

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
    pr_info("fbkms loaded successfully\n");
    return platform_driver_register(&fbkms_platform_driver);
}

static void __exit fbkms_exit(void)
{
    platform_driver_unregister(&fbkms_platform_driver);
    pr_info("fbkms exited successfully\n");
}

module_init(fbkms_init);
module_exit(fbkms_exit);

MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer KMS");
MODULE_LICENSE("GPL");
