#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#define GLOBALMEM_SIZE (1U << 10)
#define MEM_CLEAR 0x01
#define GLOBALMEM_MAJOR 200
#define GLOBALFIFO_SIZE 200

static int globalmem_major = GLOBALMEM_MAJOR;
module_param(globalmem_major, int, S_IRUGO);

struct globalmem_dev{
	struct cdev cdev;
	unsigned int current_len;
	unsigned char mem[GLOBALMEM_SIZE];
	struct mutex mutex;
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
};

struct globalmem_dev *globalmem_devp;


static int globalmem_open(struct inode *inode, struct file *filp)
{
	/*	get file private data */
	filp->private_data = globalmem_devp;
	return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct globalmem_dev *dev = filp->private_data;
	switch (cmd){
		case MEM_CLEAR:
			mutex_lock(&dev->mutex);
			memset(dev->mem, 0, GLOBALMEM_SIZE);
			mutex_unlock(&dev->mutex);
			printk(KERN_INFO "globalmem is set to zero\n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static ssize_t globalmem_read(struct file *filp, char __user * buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;
	DECLARE_WAITQUEUE(wait, current);
	
	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->r_wait, &wait);

	/* use while loop to always make sure	*/
	/* that block condition is certificated */
	/* which will occur in more than one process */
	/* being waked up by wake_up_interruptible */
	while(dev->current_len == 0){
		if (filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		/* it's important to unclock mutex */
		/*	before calling schedule       */
		mutex_unlock(&dev->mutex);

		schedule();
		if(signal_pending(current)){
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}

	if(count > dev->current_len)
		count = dev->current_len;

	if (copy_to_user(buf, dev->mem, count)){
		ret = -EFAULT;
		goto out;
	}else{
		/* Move the unread data to the start of dev->mem */
		memcpy(dev->mem, dev->mem+count, dev->current_len - count);
		dev->current_len -= count;
		printk(KERN_INFO "read %u bytes \n", count);
		
		wake_up_interruptible(&dev->w_wait);
		
		ret = count;
	}
	out:
		mutex_unlock(&dev->mutex);
	out2:
		remove_wait_queue(&dev->r_wait, &wait);
		set_current_state(TASK_RUNNING);
		return ret;

}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	DECLARE_WAITQUEUE(wait, current);
	struct globalmem_dev *dev = filp->private_data;

	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->w_wait, &wait);
	/* use while loop to always make sure	*/
	/* that block condition is certificated */
	/* which will occur in more than one process */
	/* being waked up by wake_up_interruptible */
	while(dev->current_len == GLOBALFIFO_SIZE){
		if(filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		/* it's important to unclock mutex */
		/*	before calling schedule       */
		mutex_unlock(&dev->mutex);
		schedule();
		if(signal_pending(current)){
			ret = -ERESTARTSYS;
			goto out2;
		}

		mutex_lock(&dev->mutex);
	}
	
	if(count > GLOBALFIFO_SIZE - dev->current_len)
		count = GLOBALFIFO_SIZE - dev->current_len;

	if (copy_from_user(dev->mem, buf, count)){
		ret = -EFAULT;
		goto out;
	}else{
		dev->current_len += count;
		printk(KERN_INFO "written %u bytes \n", count);
		wake_up_interruptible(&dev->r_wait);
		ret = count;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return ret;

}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret = 0;
	switch (orig){
	case 0:/* from the start of file*/
		if (offset < 0){
			ret = -EINVAL;
			break;
		}
		if ((unsigned int)offset > GLOBALMEM_SIZE){
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1: /* from the current position of file*/
		if(( filp->f_pos + offset) > GLOBALMEM_SIZE){
			ret = -EINVAL;
			break;
		}
		if ( (filp->f_pos + offset) < 0){
			ret = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,
};

static void globalmem_setup_cdev(struct globalmem_dev *dev, int index){
	
	int err, devno = MKDEV(globalmem_major, index);
	cdev_init(&dev->cdev, &globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1); // register cdev to kobj_map()

	if (err)
		printk(KERN_NOTICE "error %d adding globalmem %d", err, index);
}

static int __init globalmem_init(void)
{
	int ret;
	dev_t devno = MKDEV(globalmem_major, 0);

	/* Register device number */
	if(globalmem_major)
		ret = register_chrdev_region(devno, 1, "globalmem");
	/* If don't know the region of number, dynamically allocate it*/
	else{
		ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}

	if (ret < 0)
		return ret;
	/* Allocate memory for file private data */
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
	if (!globalmem_devp){
		ret = -ENOMEM;
		goto fail_malloc;
	}
	mutex_init(&globalmem_devp->mutex);
	init_waitqueue_head(&globalmem_devp->r_wait);
	init_waitqueue_head(&globalmem_devp->w_wait);
	globalmem_setup_cdev(globalmem_devp, 0);
	return 0;

	fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;

	return 0;
}





static void __exit hello_exit(void)
{
	cdev_del(&globalmem_devp->cdev);	// unregister cdev from kobj_map()
	kfree(globalmem_devp);				// free allocated memory of file private data 
	unregister_chrdev_region(MKDEV(globalmem_major,0), 1); // unregister device number
	printk(KERN_ALERT "driver unloaded\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(globalmem_init);
module_exit(hello_exit);
