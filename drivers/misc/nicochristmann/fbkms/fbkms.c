#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/fb.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_simple_kms_helper.h>

#define DRIVER_NAME "fbkms"

static struct drm_device *fbkms_drm;

static int fbkms_enable_vblank(struct drm_crtc *crtc) { return 0; }
static void fbkms_disable_vblank(struct drm_crtc *crtc) {}

static const struct drm_crtc_funcs fbkms_crtc_funcs = {
    .set_config = drm_atomic_helper_set_config,
    .page_flip = drm_atomic_helper_page_flip,
    .destroy = drm_crtc_cleanup,
    .reset = drm_atomic_helper_crtc_reset,
    .atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
    .atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

static int fbkms_display_pipe_prepare_fb(struct drm_simple_display_pipe *pipe,
                                         struct drm_plane_state *new_state)
{
    return 0;
}

static void fbkms_display_pipe_update(struct drm_simple_display_pipe *pipe,
                                      struct drm_plane_state *old_state)
{
    struct drm_plane_state *state = pipe->plane.state;
    struct drm_framebuffer *fb = state->fb;
    struct drm_gem_fb *gem_fb = drm_gem_fb_get_obj(fb, 0);
    void *vaddr;

    if (!gem_fb || !gem_fb->obj[0])
        return;

    vaddr = drm_gem_vmap(gem_fb->obj[0]);
    if (vaddr) {
        // Here you would memcpy to /dev/fb0 or similar logic
        drm_gem_vunmap(gem_fb->obj[0], vaddr);
    }
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
    .prepare_fb = fbkms_display_pipe_prepare_fb,
    .update = fbkms_display_pipe_update,
};

static const struct drm_mode_config_funcs fbkms_mode_config_funcs = {
    .fb_create = drm_gem_fb_create,
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};

static const struct drm_driver fbkms_driver = {
    .driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_ATOMIC,
    .name = DRIVER_NAME,
    .desc = "Framebuffer KMS Bridge",
    .date = "20250413",
    .major = 1,
    .minor = 0,
    .gem_free_object_unlocked = drm_gem_free_object_unlocked,
    .dumb_create = drm_gem_cma_dumb_create,
    .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
    .prime_fd_to_handle = drm_gem_prime_fd_to_handle,
    .gem_prime_import = drm_gem_prime_import,
    .gem_prime_export = drm_gem_prime_export,
};

static int fbkms_probe(struct platform_device *pdev)
{
    struct drm_device *drm;
    struct drm_display_mode *mode;
    struct drm_connector *connector;
    struct drm_encoder *encoder;
    struct drm_crtc *crtc;

    drm = drm_dev_alloc(&fbkms_driver, &pdev->dev);
    if (IS_ERR(drm))
        return PTR_ERR(drm);

    fbkms_drm = drm;

    drm_mode_config_init(drm);
    drm->mode_config.funcs = &fbkms_mode_config_funcs;

    mode = drm_mode_create(drm);
    drm_mode_set_name(mode);
    mode->type = DRM_MODE_TYPE_DRIVER;
    mode->clock = 25175;
    mode->hdisplay = 640;
    mode->hsync_start = 656;
    mode->hsync_end = 752;
    mode->htotal = 800;
    mode->vdisplay = 480;
    mode->vsync_start = 490;
    mode->vsync_end = 492;
    mode->vtotal = 525;
    mode->vrefresh = 60;

    drm_simple_display_pipe_init(drm, NULL, &fbkms_pipe_funcs,
                                 (const uint32_t[]){ DRM_FORMAT_XRGB8888 }, 1,
                                 NULL, mode);

    drm_dev_register(drm, 0);
    return 0;
}

static int fbkms_remove(struct platform_device *pdev)
{
    drm_dev_unregister(fbkms_drm);
    drm_mode_config_cleanup(fbkms_drm);
    drm_dev_put(fbkms_drm);
    return 0;
}

static const struct of_device_id fbkms_of_ids[] = {
    { .compatible = "nicochristmann,fbkms" },
    { }
};

MODULE_DEVICE_TABLE(of, fbkms_of_ids);

static struct platform_driver fbkms_platform_driver = {
    .probe = fbkms_probe,
    .remove = fbkms_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = fbkms_of_ids,
    },
};

module_platform_driver(fbkms_platform_driver);

MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("DRM KMS to FB0 Bridge");
MODULE_LICENSE("GPL");
