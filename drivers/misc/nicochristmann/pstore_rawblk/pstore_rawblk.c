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

	if (type >= PSTORE_TYPE_COUNT) {
		pr_warn("pstore_rawblk: raw_write invalid type %d\n", type);
		return -EINVAL;
	}

	if (id >= RECORDS_PER_TYPE) {
		pr_warn("pstore_rawblk: raw_write invalid id %u\n", id);
		return -EINVAL;
	}

	if (size > PSTORE_BLOCK_SIZE) {
		pr_warn("pstore_rawblk: raw_write size %zu too large\n", size);
		return -EINVAL;
	}

	pr_info("pstore_rawblk: write called, id=%u type=%d size=%zu\n", id, type, size);
	
	block = PSTORE_DATA_OFFSET + TYPE_BLOCK_OFFSET(type) + id;

	bh = __bread(bdev, PSTORE_HEADER_OFFSET, PSTORE_BLOCK_SIZE);
	if (!bh) {
		pr_warn("pstore_rawblk: raw_write unable to read header block\n");
		return -EIO;
	}

	hdr = (struct pstore_raw_header *)bh->b_data;

	if (hdr->magic != PSTORE_RAW_MAGIC) {
		pr_info("pstore_rawblk: formatting header in write()\n");
		memset(hdr, 0, PSTORE_BLOCK_SIZE);
		hdr->magic = PSTORE_RAW_MAGIC;
	}

	if (hdr->record_count[type] <= id)
		hdr->record_count[type] = id + 1;

	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);
	brelse(bh);

	page = alloc_page(GFP_KERNEL);
	if (!page) {
		pr_warn("pstore_rawblk: raw_write alloc_page failed\n");
		return -ENOMEM;
	}

	memcpy(page_address(page), data, size);

	bh = __bread(bdev, block, PSTORE_BLOCK_SIZE);
	if (!bh) {
		pr_warn("pstore_rawblk: raw_write unable to read data block %lld\n", block);
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

struct rawblk_context {
	u32 current_type;
	u32 current_id[PSTORE_TYPE_COUNT];
};

static void *rawblk_open(struct pstore_info *psi)
{
	struct rawblk_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	ctx->current_type = 0;
	return ctx;
}

static int rawblk_read(struct pstore_record *record)
{
	struct rawblk_context *ctx = record->private_data;
	struct buffer_head *bh;
	struct pstore_raw_header *hdr;
	enum pstore_type_id type;
	u32 id;
	loff_t block;

	if (!ctx)
		return -EINVAL;

	bh = __bread(bdev, PSTORE_HEADER_OFFSET, PSTORE_BLOCK_SIZE);
	if (!bh)
		return -EIO;

	hdr = (struct pstore_raw_header *)bh->b_data;

	while (ctx->current_type < PSTORE_TYPE_COUNT) {
		type = ctx->current_type;
		id = ctx->current_id[type];

		if (id < hdr->record_count[type]) {
			ctx->current_id[type]++;
			brelse(bh);

			block = PSTORE_DATA_OFFSET + TYPE_BLOCK_OFFSET(type) + id;
			bh = __bread(bdev, block, PSTORE_BLOCK_SIZE);
			if (!bh)
				return -EIO;

			record->buf = kmemdup(bh->b_data, PSTORE_BLOCK_SIZE, GFP_KERNEL);
			record->size = PSTORE_BLOCK_SIZE;
			record->type = type;
			record->id = id;
			record->time = ns_to_timespec64(ktime_get_real_ns());
			record->compressed = false;
			record->private_data = ctx;

			brelse(bh);
			return record->size;
		}

		ctx->current_type++;
	}

	brelse(bh);
	return -ENODATA;
}

static void rawblk_close(struct pstore_info *psi, void *private)
{
	kfree(private);
}

static int raw_pstore_write(struct pstore_record *record)
{
	int ret;

	if (!record) {
		pr_warn("pstore_rawblk: raw_pstore_write called with NULL record\n");
		return -EINVAL;
	}
	if (!record->buf) {
		pr_warn("pstore_rawblk: raw_pstore_write record buf is NULL\n");
		return -EINVAL;
	}
	if (record->size <= 0) {
		pr_warn("pstore_rawblk: raw_pstore_write invalid size %zd\n", record->size);
		return -EINVAL;
	}

	ret = raw_write(record->id, record->type, record->buf, record->size);
	if (ret < 0) {
		pr_warn("pstore_rawblk: raw_pstore_write raw_write failed with %d\n", ret);
	}
	return ret;
}

static struct pstore_info raw_backend = {
	.name       = "rawblk",
	.open       = rawblk_open,
	.read       = rawblk_read,
	.close      = rawblk_close,
	.write      = raw_pstore_write,
	.erase      = NULL,
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
