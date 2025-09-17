#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "kzlab.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sina");

struct kzlab_manager kzm;

#define MODULE_NAME "kzlab"
#define BUF_SIZE 200


static int kz_open(struct inode *inode, struct file *filp)
{
    pr_info("open called\n");

    struct kzlab_manager* kz = container_of(inode->i_cdev, struct kzlab_manager, kzc);


    filp->private_data = kz;

    return 0;
}

static ssize_t kz_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    
    int fsize = BUF_SIZE;
    struct kzlab_manager* kz = filp->private_data;

    down_interruptible(&kz->kzsem);

    if(*ppos >= BUF_SIZE)
    {
        *ppos = 0;
        up(&kz->kzsem);
        return 0;
    }

    pr_info("read happened\n");


    copy_to_user(buf, kz->kzbuffer, BUF_SIZE);
    
    *ppos = BUF_SIZE + 1;

    up(&kz->kzsem);

    return BUF_SIZE;

}

static ssize_t kz_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    pr_info("Write to device happened\n");
    int fsize = BUF_SIZE;

    struct kzlab_manager* kz = filp->private_data;
    
    down_interruptible(&kz->kzsem);

    if(size < BUF_SIZE)
    {
        fsize = size;
    }


    memset(kz->kzbuffer, 0, BUF_SIZE);

    if(copy_from_user(kz->kzbuffer, buf, fsize))
    {        
        up(&kz->kzsem);
        return -EFAULT;
    }

    up(&kz->kzsem);
    return BUF_SIZE;

}




struct file_operations fil_op = {
    .owner = THIS_MODULE,
    .open = kz_open,
    .read = kz_read,
    .write = kz_write,
};


struct class* kz_class;
struct device* kz_device;



static int __init kz_init(void)
{
    int err = 0;


    sema_init(&kzm.kzsem, 1);


    kzm.kzbuffer = kzalloc(BUF_SIZE, GFP_KERNEL);

    if(kzm.kzbuffer == NULL)
    {
        pr_alert("Buffer allocation failed\n");
        return -1;
    }
    
    err = alloc_chrdev_region(&(kzm.kzdev), 0, 1, MODULE_NAME);

    if(err)
    {
        pr_alert("chrdev allocation failed\n");
        goto clean_buffer;
    }


    cdev_init(&kzm.kzc, &fil_op);

    kzm.kzc.owner = THIS_MODULE;

    err = cdev_add(&kzm.kzc, kzm.kzdev, 1);

    if(err)
    {
        pr_alert("cdev adding failed\n");
        goto unreg_chrdev;
    }

    kz_class = class_create(MODULE_NAME);

    if(IS_ERR(kz_class))
    {
        pr_alert("class creation failed\n");
        goto cdev_del;
    }

    kz_device = device_create(kz_class, NULL, kzm.kzdev, NULL, MODULE_NAME);

    if(IS_ERR(kz_device))
    {
        pr_alert("device creation failed\n");
        goto class_destroy;
    }


    pr_info("Initialization complete\n");

    goto ok;


    class_destroy:
        class_destroy(kz_class);
    cdev_del:
        cdev_del(&kzm.kzc);
    unreg_chrdev:
        unregister_chrdev_region(kzm.kzdev, 1);
    clean_buffer:
        kfree(kzm.kzbuffer);


    ok:
    return err;
}


static void __exit kz_exit(void)
{

    device_destroy(kz_class, kzm.kzdev);
    class_destroy(kz_class);
    cdev_del(&kzm.kzc);
    unregister_chrdev_region(kzm.kzdev, 1);
    kfree(kzm.kzbuffer);

    pr_info("kzlab unloaded\n");
}


module_init(kz_init);
module_exit(kz_exit);