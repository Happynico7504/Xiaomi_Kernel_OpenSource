// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include <drm/drm_drv.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_cma_helper.h>

#define DRIVER_NAME "fbkms"
#define DRIVER_DESC "DRM-to-fb0 KMS driver"
#define DRIVER_DATE "20250413"
#define DRIVER_MAJOR 1
#define DRIVER_MINOR 0

struct fbkms_device {
	struct drm_device drm;
	struct drm_simple_display_pipe pipe;
	struct fb_info *fbinfo;
};

static int fbkms_pipe_enable(struct drm_simple_display_pipe *pipe,
			   struct drm_crtc_state *crtc_state,
			   struct drm_plane_state *plane_state)
{
	struct drm_framebuffer *fb = plane_state->fb;
	struct drm_gem_cma_object *obj;
	void *src;
	struct fbkms_device *fbkms = container_of(pipe->crtc.dev, struct fbkms_device, drm);

	if (!fb || !fbkms->fbinfo || !fb->obj[0])
		return -EINVAL;

	obj = drm_fb_cma_get_gem_obj(fb, 0);
	src = obj->vaddr;

	memcpy(fbkms->fbinfo->screen_base, src, fb->height * fb->pitches[0]);

	return 0;
}

static void fbkms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	// No-op
}

static const struct drm_simple_display_pipe_funcs fbkms_pipe_funcs = {
	.enable = fbkms_pipe_enable,
	.disable = fbkms_pipe_disable,
};

static const struct drm_mode_config_funcs fbkms_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static struct drm_driver fbkms_driver = {
	.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_ATOMIC,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	DRM_GEM_CMA_DRIVER_OPS,
};

static int fbkms_probe(struct platform_device *pdev)
{
	struct drm_device *drm;
	struct fbkms_device *fbkms;
	struct drm_display_mode *mode;
	struct drm_connector *connector;
	static const u32 formats[] = { DRM_FORMAT_XRGB8888 };
	int ret;

	fbkms = devm_kzalloc(&pdev->dev, sizeof(*fbkms), GFP_KERNEL);
	if (!fbkms)
		return -ENOMEM;

	drm = &fbkms->drm;

	ret = devm_drm_dev_init(&pdev->dev, drm, &fbkms_driver);
	if (ret)
		return ret;

	drm_mode_config_init(drm);
	drm->mode_config.funcs = &fbkms_config_funcs;
	drm->mode_config.min_width = 720;
	drm->mode_config.max_width = 720;
	drm->mode_config.min_height = 1600;
	drm->mode_config.max_height = 1600;

	mode = drm_mode_create(drm);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	mode->clock = 60000;
	mode->hdisplay = 720;
	mode->hsync_start = 740;
	mode->hsync_end = 760;
	mode->htotal = 800;
	mode->vdisplay = 1600;
	mode->vsync_start = 1610;
	mode->vsync_end = 1620;
	mode->vtotal = 1650;

	ret = drm_simple_display_pipe_init(drm, &fbkms->pipe, &fbkms_pipe_funcs,
					formats, ARRAY_SIZE(formats), NULL, mode);
	if (ret)
		return ret;

	connector = &fbkms->pipe.connector;
	ret = drm_connector_attach_encoder(connector, &fbkms->pipe.encoder);
	if (ret)
		return ret;

	drm_mode_config_reset(drm);
	platform_set_drvdata(pdev, fbkms);

	fbkms->fbinfo = registered_fb[0]; // assumes fb0 exists
	return drm_dev_register(drm, 0);
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
		.name = DRIVER_NAME,
	},
};

module_platform_driver(fbkms_platform_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Nico Christmann");
MODULE_LICENSE("GPL");
