/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/version.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_gem_cma_helper.h>
/* Dein Kernel hat drm_fb_cma_helper.h, wir inkludieren es falls vorhanden */
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_modeset_helper.h>
#include <drm/drm_connector.h>
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

/* --- Modes provider: erzeugt einen einfachen Mode basierend auf fbdev-Auflösung --- */
static int fbkms_get_modes(struct drm_connector *connector)
{
    struct drm_display_mode tmp = { 0 };
    struct drm_display_mode *mode;
    struct drm_device *drm = connector->dev;
    struct fbkms_device *fbkms = container_of(connector, struct fbkms_device, connector);
    struct fb_info *info = fbkms->fb;
    unsigned int h, v;

    if (!info)
        return 0;

    /* benutze fbdev-Auflösung, falls vorhanden, sonst Fallback */
    h = info->var.xres ?: 800;
    v = info->var.yres ?: 600;

    /* Fülle einen temporären mode struct und dupliziere ihn für das DRM-Core */
    tmp.hdisplay = h;
    tmp.hsync_start = h + 1;
    tmp.hsync_end = h + 1;
    tmp.htotal = h + 1;

    tmp.vdisplay = v;
    tmp.vsync_start = v + 1;
    tmp.vsync_end = v + 1;
    tmp.vtotal = v + 1;

    tmp.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

    /* setze den Namen (helper) */
#if defined(drm_mode_set_name)
    drm_mode_set_name(&tmp);
#else
    /* Falls drm_mode_set_name nicht vorhanden ist, setze kein Namenfeld */
#endif

    mode = drm_mode_duplicate(drm->dev, &tmp);
    if (!mode)
        return 0;

    drm_mode_probed_add(connector, mode);
    return 1;
}

/* --- Connector detect / funcs / helper funcs --- */
static enum drm_connector_status fbkms_detect(struct drm_connector *connector, bool force)
{
    return connector_status_connected;
}

static const struct drm_connector_funcs fbkms_conn_funcs = {
    .reset = drm_atomic_helper_connector_reset,
    .detect = fbkms_detect,
    /* .fill_modes ist älter; wir verwenden get_modes über helper funcs */
    .fill_modes = NULL,
    .destroy = drm_connector_cleanup,
    .atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs fbkms_conn_helper_funcs = {
    .get_modes = fbkms_get_modes,
    .best_encoder = NULL,
};

/* --- simple display pipe callbacks --- */
static void fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
                              struct drm_crtc_state *crtc_state,
                              struct drm_plane_state *plane_state)
{
    struct drm_device *drm = pipe->crtc.dev;
    struct fbkms_device *fbkms = drm_to_fbkms(drm);
    struct drm_framebuffer *fb = plane_state ? plane_state->fb : NULL;
    struct drm_gem_cma_object *cma_obj;
    void *src = NULL;
    size_t copy_bytes = 0;

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

/* --- mode_config funcs: kein fb_create (mtkfb stellt fbdev bereit) --- */
static const struct drm_mode_config_funcs fbkms_mode_config_funcs = {
    .fb_create = NULL,
#if defined(drm_kms_helper_hotplug_event)
    .output_poll_changed = drm_kms_helper_hotplug_event,
#endif
};

/* --- drm_driver --- */
static struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM,
    .name = "fbkms",
    .desc = "Framebuffer -> KMS Bridge",
    .date = "20250625",
    .gem_free_object_unlocked = drm_gem_cma_free_object,
    .dumb_create = drm_gem_cma_dumb_create,
    .dumb_destroy = drm_gem_dumb_destroy,
};

/* --- probe/remove --- */
static int fbkms_probe(struct platform_device *pdev)
{
    struct fbkms_device *fbkms;
    struct fb_info *info = NULL;
    int ret;
    int i;
    struct drm_device *drm;
    struct device *dev = &pdev->dev;

    dev_info(dev, "fbkms: probe start\n");

    fbkms = devm_kzalloc(dev, sizeof(*fbkms), GFP_KERNEL);
    if (!fbkms)
        return -ENOMEM;

    /* framebuffer suchen */
    for (i = 0; i < FB_MAX; i++) {
        if (registered_fb[i] && registered_fb[i]->screen_base) {
            info = registered_fb[i];
            break;
        }
    }

    if (!info) {
        dev_err(dev, "fbkms: no usable fbdev found\n");
        return -ENODEV;
    }

    fbkms->fb = info;

    ret = drm_dev_init(&fbkms->drm, &fbkms_driver, dev);
    if (ret) {
        dev_err(dev, "fbkms: drm_dev_init failed: %d\n", ret);
        return ret;
    }

    drm = &fbkms->drm;

    drm_mode_config_init(drm);
    drm->mode_config.min_width  = 1;
    drm->mode_config.min_height = 1;
    drm->mode_config.max_width  = info->var.xres_virtual ?: 3840;
    drm->mode_config.max_height = info->var.yres_virtual ?: 2160;
    drm->mode_config.funcs = &fbkms_mode_config_funcs;

    dev_info(dev, "fbkms: mode_config initialized (max %ux%u)\n",
             drm->mode_config.max_width, drm->mode_config.max_height);

    ret = drm_simple_display_pipe_init(drm,
                                       &fbkms->pipe,
                                       &fbkms_pipe_funcs,
                                       fbkms_formats, ARRAY_SIZE(fbkms_formats),
                                       NULL,
                                       &fbkms->connector);
    if (ret) {
        dev_err(dev, "fbkms: drm_simple_display_pipe_init failed: %d\n", ret);
        goto err_mode_config;
    }

    drm_connector_helper_add(&fbkms->connector, &fbkms_conn_helper_funcs);

    ret = drm_connector_attach_encoder(&fbkms->connector, &fbkms->pipe.encoder);
    if (ret) {
        dev_err(dev, "fbkms: drm_connector_attach_encoder failed: %d\n", ret);
        goto err_pipe;
    }

    fbkms->connector.polled = DRM_CONNECTOR_POLL_CONNECT;
    /* dpms wird intern verwaltet, aber falls dein kernel es erwartet: */
#if defined(DRM_MODE_DPMS_ON)
    fbkms->connector.dpms = DRM_MODE_DPMS_ON;
#endif
    fbkms->connector.display_info.width_mm = 68;
    fbkms->connector.display_info.height_mm = 122;

    platform_set_drvdata(pdev, fbkms);

    ret = drm_dev_register(drm, 0);
    if (ret) {
        dev_err(dev, "fbkms: drm_dev_register failed: %d\n", ret);
        goto err_pipe;
    }

    drm_kms_helper_poll_init(drm);

    dev_info(dev, "fbkms: registered (using fbdev %s)\n", fbkms->fb->fix.id);
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

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_probe,
    .remove = fbkms_remove,
    .driver = {
        .name = "fbkms",
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
MODULE_DESCRIPTION("Framebuffer -> KMS Bridge Layer");
MODULE_LICENSE("GPL");
