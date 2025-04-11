#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

extern int primary_display_trigger(bool blocking, void *callback, unsigned int userdata);

static struct timer_list fbflush_timer;

static void fbflush_callback(struct timer_list *t)
{
    primary_display_trigger(true, NULL, 0);
    mod_timer(&fbflush_timer, jiffies + msecs_to_jiffies(10));
}

static int __init fbflush_init(void)
{
    pr_info("fbflush: starting display refresh timer\n");
    timer_setup(&fbflush_timer, fbflush_callback, 0);
    mod_timer(&fbflush_timer, jiffies + msecs_to_jiffies(1000));
    return 0;
}

static void __exit fbflush_exit(void)
{
    del_timer_sync(&fbflush_timer);
    pr_info("fbflush: stopped\n");
}

module_init(fbflush_init);
module_exit(fbflush_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nico Christmann");
MODULE_DESCRIPTION("Framebuffer Refresh Timer");
