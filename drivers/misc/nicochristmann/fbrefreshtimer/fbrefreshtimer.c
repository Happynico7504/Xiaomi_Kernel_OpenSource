#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

extern int primary_display_trigger(bool blocking, void *callback, unsigned int userdata);

static struct delayed_work fbflush_work;

static void fbflush_work_func(struct work_struct *work)
{
    primary_display_trigger(true, NULL, 0);
    schedule_delayed_work(&fbflush_work, msecs_to_jiffies(5));
}

static int __init fbflush_init(void)
{
    pr_info("fbflush: scheduling display refresh work\n");
    INIT_DELAYED_WORK(&fbflush_work, fbflush_work_func);
    schedule_delayed_work(&fbflush_work, msecs_to_jiffies(5));
    return 0;
}

static void __exit fbflush_exit(void)
{
    cancel_delayed_work_sync(&fbflush_work);
    pr_info("fbflush: stopped\n");
}

module_init(fbflush_init);
module_exit(fbflush_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer Refresh Timer");
