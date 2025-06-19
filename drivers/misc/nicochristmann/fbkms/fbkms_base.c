#include <linux/module.h>

#include "fbkms_sdk.h"

static int __init fbkms_init(void)
{
    pr_info("fbkms loaded successfully\n");
    return platform_driver_register(&fbkms_platform_driver);
}

static void __exit fbkms_exit(void)
{
    platform_driver_unregister(&fbkms_platform_driver);
    pr_info("fbkms exited successfully\n");
}

module_init(fbkms_init);
module_exit(fbkms_exit);

MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer KMS");
MODULE_LICENSE("GPL");
