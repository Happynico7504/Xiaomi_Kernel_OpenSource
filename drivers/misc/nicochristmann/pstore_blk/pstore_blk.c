/* SPDX-License-Identifier: GPL-2.0 / /

Implements pstore backend driver that writes to block (or non-block)

storage devices, using the pstore/zone API.

Based on initial patch from OpenHarmony:

https://lists.openatom.io/hyperkitty/list/kernel@openharmony.io/thread/FGGOF2PMPVAVZBS2A6GIT264RKLN44MO/ */


#include <linux/kernel.h> #include <linux/module.h> #include <linux/init.h> #include <linux/fs.h> #include <linux/uaccess.h> #include <linux/slab.h> #include <linux/mount.h> #include <linux/namei.h> #include <linux/blkdev.h> #include <linux/buffer_head.h> #include <linux/uuid.h> #include <linux/vmalloc.h> #include <linux/pstore.h> #include <linux/pstore_zone.h> #include <linux/pstore_blk.h> #include <linux/of.h> #include <linux/of_platform.h>

#define PSTORE_BLK_FS_TYPE "pstoreblkfs" #define PSTORE_BLK_MAGIC  0x70737462 /* 'pstb' */

static char *pstore_blk_dev; static char *pstore_blk_mntpoint; static struct file_system_type *blkfs; static struct vfsmount *blkfs_mnt; static struct pstore_zone_info zone_info;

module_param(pstore_blk_dev, charp, 0); MODULE_PARM_DESC(pstore_blk_dev, "pstore block backend device path");

module_param(pstore_blk_mntpoint, charp, 0); MODULE_PARM_DESC(pstore_blk_mntpoint, "pstore block backend mount point");

static struct file *pstore_blk_file_open(const char *path, int flags, int mode) { struct file *filp;

filp = filp_open(path, flags, mode);
if (IS_ERR(filp)) {
    pr_err("pstore_blk: unable to open file %s, err %ld\n", path, PTR_ERR(filp));
    return NULL;
}
return filp;

}

static ssize_t pstore_blk_file_read(struct file *file, char *buf, size_t len, loff_t *pos) { mm_segment_t old_fs; ssize_t ret;

old_fs = get_fs();
set_fs(KERNEL_DS);
ret = vfs_read(file, buf, len, pos);
set_fs(old_fs);

return ret;

}

static ssize_t pstore_blk_file_write(struct file *file, const char *buf, size_t len, loff_t *pos) { mm_segment_t old_fs; ssize_t ret;

old_fs = get_fs();
set_fs(KERNEL_DS);
ret = vfs_write(file, buf, len, pos);
set_fs(old_fs);

return ret;

}

static int pstore_blk_open(struct pstore_zone_info zone) { / Initialize filesystem mount point */ struct path path; int err;

err = kern_path(pstore_blk_mntpoint, LOOKUP_DIRECTORY, &path);
if (err) {
    pr_err("pstore_blk: failed to find mountpoint %s\n", pstore_blk_mntpoint);
    return err;
}
blkfs_mnt = path.mnt;
return 0;

}

static int pstore_blk_read(struct pstore_zone_info *zone, struct pstore_record *record) { struct file *file; loff_t pos = 0; char filename[256];

snprintf(filename, sizeof(filename), "%s/%s-%lld", pstore_blk_mntpoint,
         pstore_record_type_to_name(record->type), record->id);

file = pstore_blk_file_open(filename, O_RDONLY, 0);
if (!file)
    return -ENOENT;

record->buf = vmalloc(record->size);
if (!record->buf) {
    filp_close(file, NULL);
    return -ENOMEM;
}

pstore_blk_file_read(file, record->buf, record->size, &pos);
filp_close(file, NULL);

return 0;

}

static int pstore_blk_write(struct pstore_zone_info *zone, struct pstore_record *record) { struct file *file; loff_t pos = 0; char filename[256];

snprintf(filename, sizeof(filename), "%s/%s-%lld", pstore_blk_mntpoint,
         pstore_record_type_to_name(record->type), record->id);

file = pstore_blk_file_open(filename, O_WRONLY | O_CREAT, 0644);
if (!file)
    return -EIO;

pstore_blk_file_write(file, record->buf, record->size, &pos);
filp_close(file, NULL);

return 0;

}

static struct pstore_zone_backend pstore_blk_backend = { .name       = "blk", .owner      = THIS_MODULE, .open       = pstore_blk_open, .read       = pstore_blk_read, .write      = pstore_blk_write, };

static int __init pstore_blk_init(void) { int ret;

if (!pstore_blk_dev || !pstore_blk_mntpoint) {
    pr_err("pstore_blk: device and mountpoint must be specified\n");
    return -EINVAL;
}

ret = pstore_register_zone_backend(&pstore_blk_backend);
if (ret) {
    pr_err("pstore_blk: failed to register backend\n");
    return ret;
}

pr_info("pstore_blk: registered block backend for pstore\n");
return 0;

}

static void __exit pstore_blk_exit(void) { pstore_unregister_zone_backend(&pstore_blk_backend); pr_info("pstore_blk: unregistered block backend\n"); }

module_init(pstore_blk_init); module_exit(pstore_blk_exit);

MODULE_AUTHOR("OpenHarmony/Linux Community"); MODULE_LICENSE("GPL"); MODULE_DESCRIPTION("Pstore Block Backend using pstore/zone API");

