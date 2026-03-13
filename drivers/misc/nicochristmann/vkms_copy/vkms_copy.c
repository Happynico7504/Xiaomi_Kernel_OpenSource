// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/fb.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>

/* Imported from patched vkms */
extern void *vkms_last_framebuffer;
extern u32 vkms_last_width;
extern u32 vkms_last_height;
extern u32 vkms_last_pitch;

static struct hrtimer fb_blit_timer;
static ktime_t fb_period;

/* helper to copy vkms → fbdev */
static void copy_vkms_to_fbdev(void)
{
    struct fb_info *fb;
    void *dst, *src;
    size_t size;

    pr_info("vkms-fbdev bridge: copy_vkms_to_fbdev called\n");

    if (!vkms_last_framebuffer) {
        pr_info("vkms-fbdev bridge: no VKMS framebuffer available\n");
        return;
    }

    fb = registered_fb[0]; // first fbdev
    if (!fb) {
        pr_info("vkms-fbdev bridge: no fbdev registered\n");
        return;
    }
    if (!fb->screen_base) {
        pr_info("vkms-fbdev bridge: fbdev screen_base is NULL\n");
        return;
    }

    src = vkms_last_framebuffer;
    dst = fb->screen_base;

    size = vkms_last_pitch * vkms_last_height;
    pr_info("vkms-fbdev bridge: copying %zu bytes (WxH=%ux%u, pitch=%u)\n",
            size, vkms_last_width, vkms_last_height, vkms_last_pitch);

    memcpy(dst, src, size);
    pr_info("vkms-fbdev bridge: copy complete\n");
}

/* timer callback to run at 60Hz */
static enum hrtimer_restart fb_blit_timer_func(struct hrtimer *timer)
{
    pr_info("vkms-fbdev bridge: timer tick\n");

    copy_vkms_to_fbdev();

    /* re-arm the timer */
    hrtimer_forward_now(timer, fb_period);
    pr_info("vkms-fbdev bridge: timer re-armed\n");
    return HRTIMER_RESTART;
}

static int __init fb_bridge_init(void)
{
    pr_info("vkms-fbdev bridge: init\n");

    fb_period = ktime_set(0, 16666666); // 16.666 ms → 60 Hz

    hrtimer_init(&fb_blit_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    fb_blit_timer.function = fb_blit_timer_func;
    hrtimer_start(&fb_blit_timer, fb_period, HRTIMER_MODE_REL);

    pr_info("vkms-fbdev bridge: timer started for 60Hz\n");

    return 0;
}

static void __exit fb_bridge_exit(void)
{
    pr_info("vkms-fbdev bridge: exit\n");
    hrtimer_cancel(&fb_blit_timer);
    pr_info("vkms-fbdev bridge: timer canceled\n");
}

module_init(fb_bridge_init);
module_exit(fb_bridge_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico");
MODULE_DESCRIPTION("Simple bridge: vkms → fbdev at 60Hz with full debug logs");
