// SPDX-License-Identifier: GPL-2.0+
#include "vkms_drv.h"
#include <drm/drm_plane_helper.h>
#include <drm/drm_atomic_helper.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

/* exported framebuffer info */
void *vkms_last_framebuffer;
EXPORT_SYMBOL(vkms_last_framebuffer);

u32 vkms_last_width;
EXPORT_SYMBOL(vkms_last_width);

u32 vkms_last_height;
EXPORT_SYMBOL(vkms_last_height);

u32 vkms_last_pitch;
EXPORT_SYMBOL(vkms_last_pitch);

/* primary plane atomic update */
static void vkms_primary_plane_update(struct drm_plane *plane,
                                      struct drm_plane_state *old_state)
{
    struct vkms_output *out = drm_crtc_to_vkms_output(plane->crtc);
    struct vkms_gem_object *obj;
    void *vaddr = NULL;
    int npages;

    if (!old_state || !plane->state || !plane->state->fb)
        return;

    /* obtain GEM object of framebuffer */
    obj = plane->state->fb->obj[0].driver_private;
    if (!obj)
        return;

    /* map GEM pages to kernel virtual address */
    npages = obj->gem.size / PAGE_SIZE;
    vaddr = vmap(obj->pages, npages, VM_MAP, PAGE_KERNEL);
    if (!vaddr)
        return;

    /* update last framebuffer info */
    out->last_framebuffer = vaddr;
    out->last_width  = plane->state->fb->width;
    out->last_height = plane->state->fb->height;
    out->last_pitch  = plane->state->fb->pitches[0];

    vkms_last_framebuffer = out->last_framebuffer;
    vkms_last_width = out->last_width;
    vkms_last_height = out->last_height;
    vkms_last_pitch = out->last_pitch;

/* plane helper functions */
static const struct drm_plane_helper_funcs vkms_primary_helper_funcs = {
    .atomic_update = vkms_primary_plane_update,
};

/* plane callbacks */
static const struct drm_plane_funcs vkms_plane_funcs = {
    .update_plane           = drm_atomic_helper_update_plane,
    .disable_plane          = drm_atomic_helper_disable_plane,
    .destroy                = drm_plane_cleanup,
    .reset                  = drm_atomic_helper_plane_reset,
    .atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
    .atomic_destroy_state   = drm_atomic_helper_plane_destroy_state,
};

/* initialize primary plane */
struct drm_plane *vkms_plane_init(struct vkms_device *vkmsdev)
{
    struct drm_device *dev = &vkmsdev->drm;
    struct drm_plane *plane;
    const u32 *formats;
    int ret, nformats;

    plane = kzalloc(sizeof(*plane), GFP_KERNEL);
    if (!plane)
        return ERR_PTR(-ENOMEM);

    formats = vkms_formats;
    nformats = ARRAY_SIZE(vkms_formats);

    ret = drm_universal_plane_init(dev, plane, 0,
                                   &vkms_plane_funcs,
                                   formats, nformats,
                                   NULL, DRM_PLANE_TYPE_PRIMARY, NULL);
    if (ret) {
        kfree(plane);
        return ERR_PTR(ret);
    }

    drm_plane_helper_add(plane, &vkms_primary_helper_funcs);

    return plane;
}
