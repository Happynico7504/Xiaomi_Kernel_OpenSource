#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fb.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_simple_kms_helper.h>

struct fbkms_device {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct drm_connector connector;
    struct fb_info *fb;
};

static inline struct fbkms_device *drm_to_fbkms(struct drm_device *drm)
{
    return container_of(drm, struct fbkms_device, drm);
}

static const struct drm_mode_config_funcs fbkms_mode_config_funcs = {
    .fb_create = drm_gem_cma_create,
};

static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{
    struct drm_device *drm = pipe->crtc.dev;
    struct fbkms_device *fbkms = drm_to_fbkms(drm);
    struct drm_framebuffer *fb = plane_state->fb;
    struct drm_gem_cma_object *cma_obj;
    void *src;
    size_t copy_bytes;

    if (!fb) {
        dev_err(drm->dev, "fbkms: enable called with no framebuffer\n");
        return;
    }

    if (!fb->obj[0]) {
        dev_err(drm->dev, "fbkms: fb->obj[0] is NULL\n");
        return;
    }

    cma_obj = to_drm_gem_cma_obj(fb->obj[0]);
    if (!cma_obj) {
        dev_err(drm->dev, "fbkms: framebuffer object is not CMA GEM\n");
        return;
    }

    src = cma_obj->vaddr;
    if (!src) {
        dev_err(drm->dev, "fbkms: CMA object has no vaddr mapped\n");
        return;
    }

    if (!fbkms->fb || !fbkms->fb->screen_base) {
        dev_err(drm->dev, "fbkms: target fbdev or screen_base missing\n");
        return;
    }

    copy_bytes = (size_t)fb->height * fb->pitches[0];

    if (copy_bytes > (size_t)fbkms->fb->fix.smem_len) {
        dev_warn(drm->dev, "fbkms: copy size %zu exceeds fbdev buffer %u, truncating\n",
                 copy_bytes, fbkms->fb->fix.smem_len);
        copy_bytes = fbkms->fb->fix.smem_len;
    }

    memcpy(fbkms->fb->screen_base, src, copy_bytes);

    dev_dbg(drm->dev, "fbkms: pipe enabled — copied %zu bytes to fbdev\n", copy_bytes);
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
    struct drm_device *drm = pipe->crtc.dev;
    dev_dbg(drm->dev, "fbkms: pipe disabled\n");
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .enable = fbkms_pipe_enable,
    .disable = fbkms_pipe_disable,
};

static const uint32_t fbkms_formats[] = {
    DRM_FORMAT_ARGB8888,
};

static int fbkms_probe(struct platform_device *pdev)
{
    struct fbkms_device *fbkms;
    struct fb_info *info = NULL;
    int ret;
    struct drm_device *drm;

    dev_info(&pdev->dev, "fbkms: probe start\n");

    fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms)
        return -ENOMEM;

    for (int i = 0; i < FB_MAX; i++) {
        if (registered_fb[i] && registered_fb[i]->screen_base) {
            info = registered_fb[i];
            break;
        }
    }

    if (!info) {
        dev_err(&pdev->dev, "fbkms: no usable fbdev found\n");
        return -ENODEV;
    }

    fbkms->fb = info;

    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, &pdev->dev);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_init failed: %d\n", ret);
        return ret;
    }

    drm = &fbkms->drm;

    drm_mode_config_init(drm);
    drm->mode_config.min_width  = 1;
    drm->mode_config.min_height = 1;
    drm->mode_config.max_width  = info->var.xres_virtual ?: 1920;
    drm->mode_config.max_height = info->var.yres_virtual ?: 1080;
    drm->mode_config.funcs = &fbkms_mode_config_funcs;

    dev_info(&pdev->dev, "fbkms: mode_config initialized (max %ux%u)\n",
             drm->mode_config.max_width, drm->mode_config.max_height);

    ret = drm_simple_display_pipe_init(drm,
                                       &fbkms->pipe,
                                       &fbkms_pipe_funcs,
                                       fbkms_formats, ARRAY_SIZE(fbkms_formats),
                                       NULL,
                                       &fbkms->connector);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_simple_display_pipe_init failed: %d\n", ret);
        goto err_mode_config;
    }

    fbkms->connector.display_info.width_mm = 68;
    fbkms->connector.display_info.height_mm = 122;

    platform_set_drvdata(pdev, fbkms);

    ret = drm_dev_register(drm, 0);
    if (ret) {
        dev_err(&pdev->dev, "fbkms: drm_dev_register failed: %d\n", ret);
        goto err_pipe;
    }

    drm_kms_helper_poll_init(drm);

    dev_info(&pdev->dev, "fbkms: registered (using fbdev %s)\n", fbkms->fb->fix.id);
    return 0;

err_pipe:
    drm_mode_config_cleanup(drm);
err_mode_config:
    drm_dev_put(drm);
    return ret;
}

static int fbkms_remove(struct platform_device *pdev)
{
    struct fbkms_device *fbkms = platform_get_drvdata(pdev);
    struct drm_device *drm;

    if (!fbkms)
        return -ENODEV;

    drm = &fbkms->drm;

    dev_info(&pdev->dev, "fbkms: remove start\n");

    drm_kms_helper_poll_fini(drm);
    drm_dev_unregister(drm);
    drm_mode_config_cleanup(drm);
    drm_dev_put(drm);

    dev_info(&pdev->dev, "fbkms: removed\n");
    return 0;
}

static struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM,
    .name = "fbkms",
    .desc = "Framebuffer -> KMS Bridge",
    .date = "20250625",
    .gem_free_object_unlocked = drm_gem_cma_free_object,
    .dumb_create = drm_gem_cma_dumb_create,
    .dumb_destroy = drm_gem_dumb_destroy,
};

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_probe,
    .remove = fbkms_remove,
    .driver = {
        .name = "fbkms",
        .owner = THIS_MODULE,
    },
};

static int __init fbkms_init(void)
{
    pr_info("fbkms: module init\n");
    return platform_driver_register(&fbkms_platform_driver);
}

static void __exit fbkms_exit(void)
{
    pr_info("fbkms: module exit\n");
    platform_driver_unregister(&fbkms_platform_driver);
}

module_init(fbkms_init);
module_exit(fbkms_exit);

MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer -> KMS Bridge Layer with Atomic Support");
MODULE_LICENSE("GPL");
