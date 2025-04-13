#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *fbkms_pdev;

static int __init fbkms_dev_init(void)
{
    fbkms_pdev = platform_device_register_simple("fbkms", -1, NULL, 0);
    return PTR_ERR_OR_ZERO(fbkms_pdev);
}

static void __exit fbkms_dev_exit(void)
{
    platform_device_unregister(fbkms_pdev);
}

module_init(fbkms_dev_init);
module_exit(fbkms_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Platform device for fbkms");
