#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

unsigned int default_size = 128;

MODULE_AUTHOR("Chao Jiang");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
	struct aesd_dev *data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = data;  
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */

	struct aesd_dev *data = (struct aesd_dev *)filp->private_data;
	if (mutex_lock_interruptible(&data->lock))
		return -ERESTARTSYS;

	size_t *entry_offset_byte_rtn = NULL;
	struct aesd_buffer_entry *entry = NULL;

	loff_t offp = *f_pos;
	size_t p_pos = 0;
	size_t copy_size = 0;
	do{
		entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(data->cirBuf), offp, entry_offset_byte_rtn);
		if(entry == NULL)
			break;

		copy_size = (p_pos + entry->size) >= count ? count - p_pos : entry->size;
		copy_to_user(buf + p_pos, entry->buffptr, copy_size);
		offp += copy_size;
		p_pos += copy_size;
		retval += copy_size;

	}while((p_pos + entry->size) < count);
	if(retval > 0)
	{
		printk("In Write, retval is %d and count is %d", retval, count);
	}

out:
	mutex_unlock(&data->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
	struct aesd_dev *data = (struct aesd_dev *)filp->private_data;
	if (mutex_lock_interruptible(&data->lock))
		return -ERESTARTSYS;

	int offset = *f_pos;
	if(data->buf_entry == NULL)
	{
		data->buf_entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
		if(data->buf_entry == NULL)
		{
			retval = -EFAULT;
			goto out;
		}
		memset(data->buf_entry, 0, default_size);
		data->buf_entry->buffptr = kmalloc(default_size, GFP_KERNEL);
		if(data->buf_entry->buffptr == NULL)
		{
			retval = -EFAULT;
			goto out;
		}
		memset(data->buf_entry->buffptr, 0, default_size);
	}


	if(data->buf_entry->size >= default_size)
		goto out;
	if (data->buf_entry->size + count > default_size)
		count = default_size - data->buf_entry->size;

	if (copy_from_user(data->buf_entry->buffptr + offset, buf, count)) {
		retval = -EFAULT;
		goto out;
	}

	if (buf[count -1 ] == '\n')
	{
		if(!aesd_circular_buffer_add_entry(&(data->cirBuf), data->buf_entry))
			goto out;
		*f_pos = 0;
		kfree(data->buf_entry->buffptr);
		data->buf_entry->buffptr = NULL;
		kfree(data->buf_entry);
		data->buf_entry = NULL;
		printk("In write added entry to cirBuf\n");
	}
	else
	{
		data->buf_entry->size += count;
		*f_pos = offset + count;
		printk("In write saved in buf\n");
	}

out:
	mutex_unlock(&data->lock);
    return retval;
}


struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }

    /**
     * TODO: initialize the AESD specific portion of the device
     */
	memset(&aesd_device,0,sizeof(struct aesd_dev));
	mutex_init(&(aesd_device.lock));
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
	struct aesd_buffer_entry *entryptr = NULL;
	int index = 0;
	AESD_CIRCULAR_BUFFER_FOREACH(entryptr,&(aesd_device.cirBuf),index) {
		if(entryptr)
		{
			if(entryptr->buffptr)
			{	
				kfree(entryptr->buffptr);
			}
			kfree(entryptr);
			entryptr = NULL;
		}
	}
    unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module)
