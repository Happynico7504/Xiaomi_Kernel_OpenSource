// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drmP.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico");
MODULE_DESCRIPTION("Minimal KMS + FBDEV 60FPS single-file demo");

struct mini_kms_dev {
    struct drm_device drm;
    struct drm_simple_display_pipe pipe;
    struct hrtimer vblank_timer;
    struct fb_info *fbinfo;
    void *gem_vaddr;
    u32 width;
    u32 height;
};

static struct mini_kms_dev *mk;

/* --- Minimal FBDEV ops --- */
static int mini_fb_fillrect(struct fb_info *info,
                            const struct fb_fillrect *rect)
{
    memset(info->screen_base, rect->color, info->screen_size);
    return 0;
}

static int mini_fb_copyarea(struct fb_info *info,
                            const struct fb_copyarea *area)
{
    size_t len = area->width * (info->var.bits_per_pixel / 8);
    u8 *dst = info->screen_base + area->dy * info->fix.line_length + area->dx * 4;
    u8 *src = info->screen_base + area->sy * info->fix.line_length + area->sx * 4;
    memmove(dst, src, len * area->height);
    return 0;
}

static int mini_fb_imageblit(struct fb_info *info,
                             const struct fb_image *image)
{
    u8 *dst = info->screen_base + image->dy * info->fix.line_length + image->dx * 4;
    if (copy_from_user(dst, image->data, image->height * image->width * 4))
        return -EFAULT;
    return 0;
}

static struct fb_ops mini_fb_ops = {
    .owner       = THIS_MODULE,
    .fb_fillrect = mini_fb_fillrect,
    .fb_copyarea = mini_fb_copyarea,
    .fb_imageblit= mini_fb_imageblit,
};

/* --- FBDEV init --- */
static int mini_fbdev_init(struct mini_kms_dev *mk)
{
    struct fb_info *info;

    info = framebuffer_alloc(0, mk->drm.dev);
    if (!info)
        return -ENOMEM;

    mk->width  = 720;
    mk->height = 1600;

    info->var.xres           = mk->width;
    info->var.yres           = mk->height;
    info->var.yres_virtual   = mk->height;
    info->var.bits_per_pixel = 32;

    info->screen_size = mk->width * mk->height * 4;
    info->screen_base = kzalloc(info->screen_size, GFP_KERNEL);
    if (!info->screen_base) {
        framebuffer_release(info);
        return -ENOMEM;
    }

    info->fbops = &mini_fb_ops;
    mk->fbinfo = info;

    return register_framebuffer(info);
}

/* --- VBlank timer --- */
static enum hrtimer_restart mini_vblank(struct hrtimer *timer)
{
    struct mini_kms_dev *mk = container_of(timer, struct mini_kms_dev, vblank_timer);

    /* DRM-VBlank melden */
    drm_crtc_handle_vblank(&mk->pipe.crtc);

    /* Plane-Pixel kopieren */
    if (mk->gem_vaddr && mk->fbinfo)
        memcpy(mk->fbinfo->screen_base,
               mk->gem_vaddr,
               mk->fbinfo->screen_size);

    /* Timer wieder arming für 16,666 ms (~60Hz) */
    hrtimer_forward_now(&mk->vblank_timer, ns_to_ktime(16666666));
    return HRTIMER_RESTART;
}

/* --- Mini KMS + GEM Init --- */
static int mini_kms_init(struct mini_kms_dev *mk)
{
    int ret;

    /* Simple DRM pipe init */
    ret = drm_simple_display_pipe_init(&mk->drm, &mk->pipe,
                                       &drm_simple_display_pipe_funcs,
                                       drm_formats,
                                       ARRAY_SIZE(drm_formats),
                                       NULL,
                                       DRM_PLANE_TYPE_PRIMARY,
                                       NULL);
    if (ret)
        return ret;

    /* Dummy GEM-Buffer erstellen */
    mk->gem_vaddr = kzalloc(mk->width * mk->height * 4, GFP_KERNEL);
    if (!mk->gem_vaddr)
        return -ENOMEM;

    /* Testbild: diagonale Farbverläufe */
    {
        u32 *p = mk->gem_vaddr;
        int x, y;
        for (y = 0; y < mk->height; y++) {
            for (x = 0; x < mk->width; x++) {
                p[y*mk->width + x] = ((x*255/mk->width)<<16) | ((y*255/mk->height)<<8);
            }
        }
    }

    /* FBDEV initialisieren */
    ret = mini_fbdev_init(mk);
    if (ret)
        return ret;

    /* 60Hz hrtimer starten */
    hrtimer_init(&mk->vblank_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    mk->vblank_timer.function = mini_vblank;
    hrtimer_start(&mk->vblank_timer, ns_to_ktime(16666666), HRTIMER_MODE_REL);

    return 0;
}

/* --- Modul init/exit --- */
static int __init mini_kms_module_init(void)
{
    mk = kzalloc(sizeof(*mk), GFP_KERNEL);
    if (!mk)
        return -ENOMEM;

    return mini_kms_init(mk);
}

static void __exit mini_kms_module_exit(void)
{
    hrtimer_cancel(&mk->vblank_timer);

    unregister_framebuffer(mk->fbinfo);
    kfree(mk->fbinfo->screen_base);
    framebuffer_release(mk->fbinfo);

    kfree(mk->gem_vaddr);
    kfree(mk);
}

module_init(mini_kms_module_init);
module_exit(mini_kms_module_exit);
