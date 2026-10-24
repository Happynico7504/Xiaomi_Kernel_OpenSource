// SPDX-License-Identifier: GPL-2.0
/*
 * Fake KMS driver on top of the first registered fbdev.
 *
 * Exposes one CRTC/plane/connector. Dumb buffers live in vmalloc memory and
 * are copied into fb0 on every plane update or DIRTYFB call, then the fbdev
 * is panned so drivers like mtkfb actually push the frame to the panel.
 *
 * It also registers a render node and supports PRIME, so software renderers
 * (Mesa kms_swrast) can allocate and share buffers here instead of picking
 * another render node that has no dumb buffer support.
 */

#include <linux/console.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_prime.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_vblank.h>

#define DRIVER_NAME "fakekms"

struct fakekms {
	struct drm_device *drm;
	struct platform_device *pdev;
	struct drm_simple_display_pipe pipe;
	struct drm_connector connector;

	/*
	 * Protects info, enabled and linebuf. Always taken inside console_lock,
	 * since fbmem calls our notifier with console_lock held.
	 */
	struct mutex lock;
	struct fb_info *info;
	bool enabled;
	u8 *linebuf;
	enum led_brightness backlight;	/* restored when the pipe comes back */

	unsigned int width, height;
	u32 native_format;	/* 0 if the fbdev layout has no DRM fourcc */
	u32 formats[3];
	unsigned int num_formats;
};

static struct fakekms *fakekms_dev;
static DEFINE_MUTEX(fakekms_probe_lock);

/* ---- GEM: vmalloc backed buffers, shareable through PRIME ---- */

struct fakekms_bo {
	struct drm_gem_object base;
	void *vaddr;
	struct page **pages;
	struct sg_table *sgt;	/* only set for imported buffers */
};

static inline struct fakekms_bo *to_fakekms_bo(struct drm_gem_object *obj)
{
	return container_of(obj, struct fakekms_bo, base);
}

static void fakekms_gem_free(struct drm_gem_object *obj)
{
	struct fakekms_bo *bo = to_fakekms_bo(obj);

	if (bo->sgt) {
		if (bo->vaddr)
			vunmap(bo->vaddr);
		/* not set yet if the import itself failed */
		if (obj->import_attach)
			drm_prime_gem_destroy(obj, bo->sgt);
	} else {
		vfree(bo->vaddr);
	}
	kvfree(bo->pages);
	drm_gem_object_release(obj);
	kfree(bo);
}

/* returns an empty object, the caller fills in vaddr and pages */
static struct fakekms_bo *fakekms_bo_alloc(struct drm_device *dev, size_t size)
{
	struct fakekms_bo *bo;
	int ret;

	bo = kzalloc(sizeof(*bo), GFP_KERNEL);
	if (!bo)
		return ERR_PTR(-ENOMEM);

	drm_gem_private_object_init(dev, &bo->base, size);

	/* drm_gem_prime_mmap() needs the fake offset to exist */
	ret = drm_gem_create_mmap_offset(&bo->base);
	if (ret) {
		drm_gem_object_put_unlocked(&bo->base);
		return ERR_PTR(ret);
	}

	return bo;
}

static int fakekms_dumb_create(struct drm_file *file, struct drm_device *dev,
			       struct drm_mode_create_dumb *args)
{
	unsigned int i, npages;
	struct fakekms_bo *bo;
	int ret;

	args->pitch = DIV_ROUND_UP(args->width * args->bpp, 8);
	args->size = PAGE_ALIGN((u64)args->pitch * args->height);
	npages = args->size >> PAGE_SHIFT;

	bo = fakekms_bo_alloc(dev, args->size);
	if (IS_ERR(bo))
		return PTR_ERR(bo);

	bo->vaddr = vmalloc_user(args->size);
	bo->pages = kvcalloc(npages, sizeof(*bo->pages), GFP_KERNEL);
	if (!bo->vaddr || !bo->pages) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < npages; i++)
		bo->pages[i] = vmalloc_to_page(bo->vaddr + i * PAGE_SIZE);

	ret = drm_gem_handle_create(file, &bo->base, &args->handle);
out:
	drm_gem_object_put_unlocked(&bo->base);
	return ret;
}

static struct sg_table *fakekms_gem_get_sg_table(struct drm_gem_object *obj)
{
	struct fakekms_bo *bo = to_fakekms_bo(obj);

	return drm_prime_pages_to_sg(bo->pages, obj->size >> PAGE_SHIFT);
}

static struct drm_gem_object *
fakekms_gem_import_sg_table(struct drm_device *dev,
			    struct dma_buf_attachment *attach,
			    struct sg_table *sgt)
{
	size_t size = PAGE_ALIGN(attach->dmabuf->size);
	unsigned int npages = size >> PAGE_SHIFT;
	struct fakekms_bo *bo;
	int ret;

	bo = fakekms_bo_alloc(dev, size);
	if (IS_ERR(bo))
		return ERR_CAST(bo);
	bo->sgt = sgt;

	bo->pages = kvcalloc(npages, sizeof(*bo->pages), GFP_KERNEL);
	if (!bo->pages) {
		ret = -ENOMEM;
		goto err;
	}

	ret = drm_prime_sg_to_page_addr_arrays(sgt, bo->pages, NULL, npages);
	if (ret)
		goto err;

	bo->vaddr = vmap(bo->pages, npages, VM_MAP, PAGE_KERNEL);
	if (!bo->vaddr) {
		ret = -ENOMEM;
		goto err;
	}

	return &bo->base;

err:
	drm_gem_object_put_unlocked(&bo->base);
	return ERR_PTR(ret);
}

static void *fakekms_gem_vmap(struct drm_gem_object *obj)
{
	return to_fakekms_bo(obj)->vaddr;
}

static void fakekms_gem_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	/* mapped for the whole lifetime of the object */
}

static const struct vm_operations_struct fakekms_gem_vm_ops = {
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static int fakekms_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long i, npages = vma_pages(vma);
	struct fakekms_bo *bo;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	bo = to_fakekms_bo(vma->vm_private_data);

	/* drm_gem_mmap() sets up a write-combined PFN map, we want plain pages */
	vma->vm_flags &= ~(VM_PFNMAP | VM_IO);
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	vma->vm_pgoff = 0;

	/* works for both vmalloc and imported pages, unlike remap_vmalloc_range */
	for (i = 0; i < npages; i++) {
		ret = vm_insert_page(vma, vma->vm_start + i * PAGE_SIZE,
				     bo->pages[i]);
		if (ret) {
			drm_gem_vm_close(vma);
			return ret;
		}
	}

	return 0;
}

/* ---- copying into the fbdev ---- */

static u32 fakekms_pack(const struct fb_bitfield *f, u8 v)
{
	return ((u32)v >> (8 - f->length)) << f->offset;
}

static void fakekms_convert_line(struct fakekms *fk, u8 *dst, const u8 *src,
				 u32 format, unsigned int width)
{
	const struct fb_var_screeninfo *var = &fk->info->var;
	unsigned int bpp = var->bits_per_pixel;
	u32 alpha = 0;
	unsigned int x;

	if (var->transp.length)
		alpha = GENMASK(var->transp.length - 1, 0) << var->transp.offset;

	for (x = 0; x < width; x++) {
		u8 r, g, b;
		u32 px;

		if (format == DRM_FORMAT_RGB565) {
			u16 p = ((const u16 *)src)[x];

			r = ((p >> 11) & 0x1f) << 3;
			g = ((p >> 5) & 0x3f) << 2;
			b = (p & 0x1f) << 3;
		} else {
			u32 p = ((const u32 *)src)[x];

			r = p >> 16;
			g = p >> 8;
			b = p;
		}

		px = fakekms_pack(&var->red, r) | fakekms_pack(&var->green, g) |
		     fakekms_pack(&var->blue, b) | alpha;

		switch (bpp) {
		case 16:
			((u16 *)dst)[x] = px;
			break;
		case 24:
			dst[x * 3] = px;
			dst[x * 3 + 1] = px >> 8;
			dst[x * 3 + 2] = px >> 16;
			break;
		default:
			((u32 *)dst)[x] = px;
			break;
		}
	}
}

static void fakekms_lock(struct fakekms *fk)
{
	console_lock();
	mutex_lock(&fk->lock);
}

static void fakekms_unlock(struct fakekms *fk)
{
	mutex_unlock(&fk->lock);
	console_unlock();
}

/* the fbdev ioctls call these with console_lock and the fb_info lock held */
static void fakekms_pan(struct fb_info *info)
{
	struct fb_var_screeninfo var = info->var;

	var.xoffset = 0;
	var.yoffset = 0;

	if (lock_fb_info(info)) {
		fb_pan_display(info, &var);
		unlock_fb_info(info);
	}
}

static void fakekms_blank(struct fb_info *info, int blank)
{
	if (lock_fb_info(info)) {
		fb_blank(info, blank);
		unlock_fb_info(info);
	}
}

/*
 * mtkfb's pan doesn't set up the display layer, it only triggers a refresh
 * of the existing one, and that is gone once the panel was powered down.
 * set_par configures it again like at boot.
 */
static void fakekms_unblank(struct fb_info *info)
{
	if (lock_fb_info(info)) {
		fb_blank(info, FB_BLANK_UNBLANK);
		if (info->fbops->fb_set_par)
			info->fbops->fb_set_par(info);
		unlock_fb_info(info);
	}
}

/*
 * The panel backlight is a LED that Android's power manager turns back on
 * after the panel resumes. Nothing does that on a plain Linux userspace.
 */
#define FAKEKMS_BACKLIGHT	"lcd-backlight"

extern struct rw_semaphore leds_list_lock;
extern struct list_head leds_list;

static struct led_classdev *fakekms_backlight(void)
{
	struct led_classdev *led, *found = NULL;

	down_read(&leds_list_lock);
	list_for_each_entry(led, &leds_list, node) {
		if (!strcmp(led->name, FAKEKMS_BACKLIGHT)) {
			found = led;
			break;
		}
	}
	up_read(&leds_list_lock);
	return found;
}

/* caller holds fakekms_lock() */
static void fakekms_save_backlight(struct fakekms *fk)
{
	struct led_classdev *led = fakekms_backlight();

	if (led && led->brightness)
		fk->backlight = led->brightness;
}

/* caller holds fakekms_lock() */
static void fakekms_restore_backlight(struct fakekms *fk)
{
	struct led_classdev *led = fakekms_backlight();

	if (led && !led->brightness)
		led_set_brightness(led, fk->backlight ?: led->max_brightness / 2);
}

/* caller holds fakekms_lock() */
static void fakekms_flush_locked(struct fakekms *fk, struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct fb_info *info = fk->info;
	unsigned int cpp, dst_len, y;
	struct fakekms_bo *bo;
	char __iomem *dst;
	const u8 *src;

	if (!fk->enabled || !info || !fb)
		return;

	cpp = fb->format->cpp[0];
	bo = to_fakekms_bo(drm_gem_fb_get_obj(fb, 0));
	src = bo->vaddr + fb->offsets[0] + (state->src_y >> 16) * fb->pitches[0] +
	      (state->src_x >> 16) * cpp;
	dst = info->screen_base;
	dst_len = fk->width * info->var.bits_per_pixel / 8;

	for (y = 0; y < fk->height; y++) {
		if (fb->format->format == fk->native_format) {
			memcpy_toio(dst, src, dst_len);
		} else {
			fakekms_convert_line(fk, fk->linebuf, src,
					     fb->format->format, fk->width);
			memcpy_toio(dst, fk->linebuf, dst_len);
		}
		src += fb->pitches[0];
		dst += info->fix.line_length;
	}

	fakekms_pan(info);
}

static void fakekms_flush(struct fakekms *fk, struct drm_plane_state *state)
{
	fakekms_lock(fk);
	fakekms_flush_locked(fk, state);
	fakekms_unlock(fk);
}

/* ---- framebuffers ---- */

static int fakekms_fb_dirty(struct drm_framebuffer *fb, struct drm_file *file,
			    unsigned int flags, unsigned int color,
			    struct drm_clip_rect *clips, unsigned int num_clips)
{
	struct fakekms *fk = fb->dev->dev_private;
	struct drm_plane *plane = &fk->pipe.plane;

	drm_modeset_lock(&plane->mutex, NULL);
	if (plane->state->fb == fb)
		fakekms_flush(fk, plane->state);
	drm_modeset_unlock(&plane->mutex);

	return 0;
}

static const struct drm_framebuffer_funcs fakekms_fb_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
	.dirty = fakekms_fb_dirty,
};

static struct drm_framebuffer *
fakekms_fb_create(struct drm_device *dev, struct drm_file *file,
		  const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return drm_gem_fb_create_with_funcs(dev, file, mode_cmd,
					    &fakekms_fb_funcs);
}

static const struct drm_mode_config_funcs fakekms_mode_config_funcs = {
	.fb_create = fakekms_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

/* ---- display pipe ---- */

static enum drm_mode_status
fakekms_pipe_mode_valid(struct drm_crtc *crtc, const struct drm_display_mode *mode)
{
	struct fakekms *fk = crtc->dev->dev_private;

	if (mode->hdisplay != (int)fk->width || mode->vdisplay != (int)fk->height)
		return MODE_BAD;
	return MODE_OK;
}

static void fakekms_pipe_enable(struct drm_simple_display_pipe *pipe,
				struct drm_crtc_state *crtc_state,
				struct drm_plane_state *plane_state)
{
	struct fakekms *fk = pipe->crtc.dev->dev_private;

	fakekms_lock(fk);
	fk->enabled = true;
	if (fk->info)
		fakekms_unblank(fk->info);
	fakekms_flush_locked(fk, plane_state);
	fakekms_restore_backlight(fk);
	fakekms_unlock(fk);
}

static void fakekms_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	struct fakekms *fk = pipe->crtc.dev->dev_private;

	fakekms_lock(fk);
	fk->enabled = false;
	fakekms_save_backlight(fk);
	if (fk->info)
		fakekms_blank(fk->info, FB_BLANK_POWERDOWN);
	fakekms_unlock(fk);
}

static void fakekms_pipe_update(struct drm_simple_display_pipe *pipe,
				struct drm_plane_state *old_state)
{
	struct fakekms *fk = pipe->crtc.dev->dev_private;
	struct drm_crtc *crtc = &pipe->crtc;

	fakekms_flush(fk, pipe->plane.state);

	/* no vblank irq, complete the event right away */
	if (crtc->state->event) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);
		crtc->state->event = NULL;
	}
}

static const struct drm_simple_display_pipe_funcs fakekms_pipe_funcs = {
	.mode_valid = fakekms_pipe_mode_valid,
	.enable = fakekms_pipe_enable,
	.disable = fakekms_pipe_disable,
	.update = fakekms_pipe_update,
	.prepare_fb = drm_gem_fb_simple_display_pipe_prepare_fb,
};

/* ---- connector ---- */

static int fakekms_conn_get_modes(struct drm_connector *connector)
{
	struct fakekms *fk = connector->dev->dev_private;
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return 0;

	mode->hdisplay = mode->hsync_start = mode->hsync_end = mode->htotal = fk->width;
	mode->vdisplay = mode->vsync_start = mode->vsync_end = mode->vtotal = fk->height;
	mode->clock = DIV_ROUND_UP(fk->width * fk->height * 60, 1000);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_connector_helper_funcs fakekms_conn_helper_funcs = {
	.get_modes = fakekms_conn_get_modes,
};

static const struct drm_connector_funcs fakekms_conn_funcs = {
	.reset = drm_atomic_helper_connector_reset,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_connector_cleanup,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

/* ---- driver ---- */

static const struct file_operations fakekms_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.poll = drm_poll,
	.read = drm_read,
	.llseek = noop_llseek,
	.mmap = fakekms_mmap,
};

static struct drm_driver fakekms_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC |
			   DRIVER_PRIME | DRIVER_RENDER,
	.gem_free_object_unlocked = fakekms_gem_free,
	.gem_vm_ops = &fakekms_gem_vm_ops,
	.dumb_create = fakekms_dumb_create,
	.dumb_map_offset = drm_gem_dumb_map_offset,
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table = fakekms_gem_get_sg_table,
	.gem_prime_import_sg_table = fakekms_gem_import_sg_table,
	.gem_prime_vmap = fakekms_gem_vmap,
	.gem_prime_vunmap = fakekms_gem_vunmap,
	.gem_prime_mmap = drm_gem_prime_mmap,
	.fops = &fakekms_fops,
	.name = DRIVER_NAME,
	.desc = "Fake KMS on top of fbdev",
	.date = "20261023",
	.major = 1,
	.minor = 0,
};

static u32 fakekms_native_format(const struct fb_var_screeninfo *var)
{
	if (var->bits_per_pixel == 32 && var->red.offset == 16 &&
	    var->green.offset == 8 && var->blue.offset == 0)
		return DRM_FORMAT_XRGB8888;
	if (var->bits_per_pixel == 32 && var->red.offset == 0 &&
	    var->green.offset == 8 && var->blue.offset == 16)
		return DRM_FORMAT_XBGR8888;
	if (var->bits_per_pixel == 16 && var->red.offset == 11 &&
	    var->green.offset == 5 && var->blue.offset == 0)
		return DRM_FORMAT_RGB565;
	return 0;
}

static bool fakekms_fb_usable(const struct fb_info *info)
{
	const struct fb_var_screeninfo *var = &info->var;

	if (info->fix.visual != FB_VISUAL_TRUECOLOR || !info->screen_base)
		return false;
	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
	    var->bits_per_pixel != 32)
		return false;
	if (!var->red.length || var->red.length > 8 ||
	    !var->green.length || var->green.length > 8 ||
	    !var->blue.length || var->blue.length > 8)
		return false;
	return var->xres && var->yres;
}

static void fakekms_add_format(struct fakekms *fk, u32 format)
{
	unsigned int i;

	for (i = 0; i < fk->num_formats; i++)
		if (fk->formats[i] == format)
			return;
	fk->formats[fk->num_formats++] = format;
}

static const struct platform_device_info fakekms_pdev_info = {
	.name = DRIVER_NAME,
	.id = PLATFORM_DEVID_NONE,
	.dma_mask = DMA_BIT_MASK(64),
};

static int fakekms_attach(struct fb_info *info)
{
	struct drm_mode_config *config;
	struct platform_device *pdev;
	struct drm_device *drm;
	struct fakekms *fk;
	int ret;

	if (!fakekms_fb_usable(info)) {
		pr_err(DRIVER_NAME ": fb%d has an unsupported layout\n", info->node);
		return -EINVAL;
	}

	pdev = platform_device_register_full(&fakekms_pdev_info);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	/* a bare platform device only gets dummy DMA ops, PRIME import needs real ones */
	arch_setup_dma_ops(&pdev->dev, 0, DMA_BIT_MASK(64), NULL, false);

	drm = drm_dev_alloc(&fakekms_driver, &pdev->dev);
	if (IS_ERR(drm)) {
		ret = PTR_ERR(drm);
		goto err_pdev;
	}

	fk = kzalloc(sizeof(*fk), GFP_KERNEL);
	if (!fk) {
		ret = -ENOMEM;
		goto err_drm;
	}
	fk->drm = drm;
	fk->pdev = pdev;
	fk->info = info;
	fk->width = info->var.xres;
	fk->height = info->var.yres;
	fk->native_format = fakekms_native_format(&info->var);
	mutex_init(&fk->lock);
	drm->dev_private = fk;

	fk->linebuf = kmalloc_array(fk->width, 4, GFP_KERNEL);
	if (!fk->linebuf) {
		ret = -ENOMEM;
		goto err_fk;
	}

	if (fk->native_format)
		fakekms_add_format(fk, fk->native_format);
	fakekms_add_format(fk, DRM_FORMAT_XRGB8888);
	fakekms_add_format(fk, DRM_FORMAT_RGB565);

	drm_mode_config_init(drm);
	config = &drm->mode_config;
	config->max_width = 8192;
	config->max_height = 8192;
	config->preferred_depth = 24;
	config->prefer_shadow = 1;
	config->funcs = &fakekms_mode_config_funcs;

	drm_connector_helper_add(&fk->connector, &fakekms_conn_helper_funcs);
	ret = drm_connector_init(drm, &fk->connector, &fakekms_conn_funcs,
				 DRM_MODE_CONNECTOR_Unknown);
	if (ret)
		goto err_config;
	fk->connector.display_info.width_mm = info->var.width;
	fk->connector.display_info.height_mm = info->var.height;

	ret = drm_simple_display_pipe_init(drm, &fk->pipe, &fakekms_pipe_funcs,
					   fk->formats, fk->num_formats, NULL,
					   &fk->connector);
	if (ret)
		goto err_config;

	drm_mode_config_reset(drm);

	ret = drm_dev_register(drm, 0);
	if (ret)
		goto err_config;

	fakekms_dev = fk;
	DRM_INFO("fakekms: %ux%u on fb%d (%s)\n", fk->width, fk->height,
		 info->node, info->fix.id);
	return 0;

err_config:
	drm_mode_config_cleanup(drm);
	kfree(fk->linebuf);
err_fk:
	kfree(fk);
err_drm:
	drm_dev_put(drm);
err_pdev:
	platform_device_unregister(pdev);
	return ret;
}

static void fakekms_detach(void)
{
	struct fakekms *fk = fakekms_dev;

	if (!fk)
		return;

	drm_dev_unregister(fk->drm);
	drm_atomic_helper_shutdown(fk->drm);
	drm_mode_config_cleanup(fk->drm);
	drm_dev_put(fk->drm);
	platform_device_unregister(fk->pdev);
	kfree(fk->linebuf);
	kfree(fk);
	fakekms_dev = NULL;
}

static void fakekms_probe_work_fn(struct work_struct *work)
{
	mutex_lock(&fakekms_probe_lock);
	if (!fakekms_dev && registered_fb[0])
		fakekms_attach(registered_fb[0]);
	mutex_unlock(&fakekms_probe_lock);
}

static DECLARE_WORK(fakekms_probe_work, fakekms_probe_work_fn);

/*
 * Called with console_lock and the fb_info lock held, so only kick off the
 * attach here and never touch the fbdev synchronously.
 */
static int fakekms_fb_notify(struct notifier_block *nb, unsigned long action,
			     void *data)
{
	struct fb_event *event = data;

	switch (action) {
	case FB_EVENT_FB_REGISTERED:
		if (event->info->node == 0)
			schedule_work(&fakekms_probe_work);
		break;
	case FB_EVENT_FB_UNREGISTERED:
		/*
		 * Stop scanning out into a dead fb_info, the DRM device stays.
		 * console_lock is already held here, so taking fk->lock keeps
		 * the usual lock order.
		 */
		if (fakekms_dev && fakekms_dev->info == event->info) {
			mutex_lock(&fakekms_dev->lock);
			fakekms_dev->info = NULL;
			mutex_unlock(&fakekms_dev->lock);
		}
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block fakekms_fb_nb = {
	.notifier_call = fakekms_fb_notify,
};

static int __init fakekms_init(void)
{
	int ret;

	ret = fb_register_client(&fakekms_fb_nb);
	if (ret)
		return ret;

	/* fb0 may already be there */
	schedule_work(&fakekms_probe_work);
	return 0;
}

static void __exit fakekms_exit(void)
{
	fb_unregister_client(&fakekms_fb_nb);
	cancel_work_sync(&fakekms_probe_work);

	mutex_lock(&fakekms_probe_lock);
	fakekms_detach();
	mutex_unlock(&fakekms_probe_lock);
}

module_init(fakekms_init);
module_exit(fakekms_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Fake KMS driver on top of the first registered fbdev");
