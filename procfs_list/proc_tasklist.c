#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/version.h>

#define PROC_READ "driver/task_list"




static void *as_start(struct seq_file *m, loff_t *pos)
{
	loff_t n = *pos;
	struct task_struct *p = NULL;

	printk("%lld (%s)    pos: %d\n", n, __func__, (int)(*pos));

	if (n==0){
		seq_printf(m, "=== seq_file header ===\n");
	}

	/* Acquire a read only spinlock */
	#if(LINUX_VERSION_CODE <= KERNEL_VERSION(2,16,18))
		read_lock(&tasklist_lock);
	#else
		rcu_read_lock();
	#endif

	if(n == 0)
		return (&init_task);
	
	for_each_process(p){
		n--;
		if(n < 0)
			return (p);
	}
	return 0;
}

static void *as_next(struct seq_file *m, void *p, loff_t *pos){
	struct task_struct *tp = (struct task_struct *)p;

	printk("%lld (%s)\n", *pos, __func__);
	(*pos)++;

	tp = next_task(tp);
	if(tp == &init_task)
		return NULL;

	return tp;
}

static void as_stop(struct seq_file *m, void *p){
	printk("%p (%s)\n", p, __func__);
	#if(LINUX_VERSION_CODE <= KERNEL_VERSION(2,16,18))
		read_unlock(&tasklist_lock);
	#else
		rcu_read_unlock();
	#endif
}

static int as_show(struct seq_file *m, void *p){
	struct task_struct *tp = (struct task_struct *)p;
	printk("%p (%s)\n", tp, __func__);

	seq_printf(m, "[%s] pid=%d\n", tp->comm, tp->pid);
	seq_printf(m, "		 tgid=%d\n", tp->tgid);
	seq_printf(m, "		 state=%ld\n", tp->state);
	seq_printf(m, "		 mm=0x%p\n", tp->mm);
	seq_printf(m, "		 utime=%lu\n", tp->utime);
	seq_printf(m, "		 stime=%lu\n", tp->stime);
//	seq_printf(m, "		 oomkilladj=%d\n", tp->oomkilladj);
	seq_printf(m, "\n");

	return 0;
}


static struct seq_operations simple_seq_op = {
	.start = as_start,
	.next = as_next,
	.stop = as_stop,
	.show = as_show,
};

static int read_proc_open(struct inode *indoe, struct file *file){
	return seq_open(file, &simple_seq_op);
}

static const struct file_operations read_proc_fops = {
	.owner = THIS_MODULE,
	.open = read_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __init proc_write_init(void)
{
	struct proc_dir_entry *entry;

	/* Add /proc */
	entry = proc_create(PROC_READ, 0, NULL, &read_proc_fops);
								 
	if (entry == NULL)
		printk(KERN_WARNING "sample: unbale to create /proc entry\n");


	printk("dirver loaded\n");
	return 0;
}




static void __exit proc_read_exit(void)
{
	remove_proc_entry(PROC_READ, NULL);
	printk(KERN_ALERT "driver unloaded\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(proc_write_init);
module_exit(proc_read_exit);
