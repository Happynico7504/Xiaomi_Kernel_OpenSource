// SPDX-License-Identifier: GPL-2.0
// Raw pstore backend using a block device, per-type support

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pstore.h>
#include <linux/buffer_head.h>

#define PSTORE_RAW_MAGIC 0x50535242 // 'PSRB'
#define PSTORE_BLOCK_SIZE 512
#define RECORDS_PER_TYPE 8192
#define PSTORE_TYPE_COUNT (PSTORE_TYPE_MCE + 1)
#define PSTORE_MAX_RECORDS (RECORDS_PER_TYPE * PSTORE_TYPE_COUNT)
#define PSTORE_HEADER_OFFSET 0
#define PSTORE_DATA_OFFSET 1

#define TYPE_BLOCK_OFFSET(type) ((type) * RECORDS_PER_TYPE)

static struct module *rawblk_holder;
static struct block_device *bdev;

static char *device_path = "/dev/mmcblk1p3";
module_param(device_path, charp, 0444);
MODULE_PARM_DESC(device_path, "Path to raw block device for pstore");

struct pstore_raw_header {
	u32 magic;
	u32 record_count[PSTORE_TYPE_COUNT];
} __attribute__((packed));

static ssize_t raw_write(u32 id, enum pstore_type_id type,
                         const char *data, size_t size)
{
	struct buffer_head *bh;
	struct pstore_raw_header *hdr;
	struct page *page;
	loff_t block;

	if (type >= PSTORE_TYPE_COUNT || id >= RECORDS_PER_TYPE || size > PSTORE_BLOCK_SIZE)
		return -EINVAL;

	block = PSTORE_DATA_OFFSET + TYPE_BLOCK_OFFSET(type) + id;

	bh = __bread(bdev, PSTORE_HEADER_OFFSET, PSTORE_BLOCK_SIZE);
	if (!bh)
		return -EIO;

	hdr = (struct pstore_raw_header *)bh->b_data;

	if (hdr->magic != PSTORE_RAW_MAGIC) {
		memset(hdr, 0, PSTORE_BLOCK_SIZE);
		hdr->magic = PSTORE_RAW_MAGIC;
		pr_info("pstore_rawblk: formatting header in write()\n");
	}

	if (hdr->record_count[type] <= id)
		hdr->record_count[type] = id + 1;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	page = alloc_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	memcpy(page_address(page), data, size);

	bh = __bread(bdev, block, PSTORE_BLOCK_SIZE);
	if (!bh) {
		__free_page(page);
		return -EIO;
	}

	memcpy(bh->b_data, page_address(page), PSTORE_BLOCK_SIZE);
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);
	__free_page(page);

	return size;
}

static int raw_pstore_read(struct pstore_record *record)
{
	struct buffer_head *bh;
	struct pstore_raw_header *hdr;
	u32 id = record->id;
	enum pstore_type_id type = record->type;
	loff_t block;

	if (type >= PSTORE_TYPE_COUNT)
		return -EINVAL;

	bh = __bread(bdev, PSTORE_HEADER_OFFSET, PSTORE_BLOCK_SIZE);
	if (!bh)
		return -EIO;

	hdr = (struct pstore_raw_header *)bh->b_data;

	if (hdr->magic != PSTORE_RAW_MAGIC) {
		brelse(bh);
		return -EINVAL;
	}

	if (id >= hdr->record_count[type]) {
		brelse(bh);
		return -ENODATA;
	}

	block = PSTORE_DATA_OFFSET + TYPE_BLOCK_OFFSET(type) + id;
	brelse(bh);

	bh = __bread(bdev, block, PSTORE_BLOCK_SIZE);
	if (!bh)
		return -EIO;

	record->size = PSTORE_BLOCK_SIZE;
	record->buf = kmemdup(bh->b_data, PSTORE_BLOCK_SIZE, GFP_KERNEL);
	record->time = ns_to_timespec64(ktime_get_real_ns());
	record->compressed = false;

	brelse(bh);

	record->id = id + 1;

	return 0;
}

static int raw_pstore_write(struct pstore_record *record)
{
	return raw_write(record->id, record->type, record->buf, record->size);
}

static struct pstore_info raw_backend = {
	.name       = "rawblk",
	.read       = raw_pstore_read,
	.write      = raw_pstore_write,
	.erase      = NULL,
	.open       = NULL,
	.close      = NULL,
	.flags      = PSTORE_TYPE_DMESG |
	              PSTORE_TYPE_CONSOLE |
	              PSTORE_TYPE_PMSG |
	              PSTORE_TYPE_FTRACE |
	              PSTORE_TYPE_MCE,
};

static int __init rawblk_init(void)
{
	struct pstore_raw_header *hdr;
	struct buffer_head *bh;

	rawblk_holder = THIS_MODULE;

	bdev = blkdev_get_by_path(device_path,
	                          FMODE_READ | FMODE_WRITE | FMODE_EXCL,
	                          rawblk_holder);
	if (IS_ERR(bdev)) {
		pr_err("pstore_rawblk: cannot open %s\n", device_path);
		return PTR_ERR(bdev);
	}

	bh = __bread(bdev, PSTORE_HEADER_OFFSET, PSTORE_BLOCK_SIZE);
	if (!bh) {
		pr_err("pstore_rawblk: unable to read header block\n");
		blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
		return -EIO;
	}

	hdr = (struct pstore_raw_header *)bh->b_data;
	if (hdr->magic != PSTORE_RAW_MAGIC) {
		pr_info("pstore_rawblk: formatting storage\n");
		memset(hdr, 0, PSTORE_BLOCK_SIZE);
		hdr->magic = PSTORE_RAW_MAGIC;
		mark_buffer_dirty(bh);
		sync_dirty_buffer(bh);
	}
	brelse(bh);

	return pstore_register(&raw_backend);
}

static void __exit rawblk_exit(void)
{
	pstore_unregister(&raw_backend);
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
}

module_init(rawblk_init);
module_exit(rawblk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("pstore backend using raw block device");
