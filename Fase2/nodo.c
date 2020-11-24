#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>



MODULE_LICENSE("Dual BSD/GPL");

#define MENOR 1
#define KPMSIZE sizeof(u64)

static int minor = MENOR;

module_param(minor, int, S_IRUGO);


bool dynamic =true;
dev_t device_number;
struct class *my_class;
struct cdev my_cdev[MENOR];


static int stat_seq_open(struct inode *inode, struct file *filp) {
	unsigned int *ptr;
	struct seq_file *file_open;
    unsigned size=4096*(1+num_possible_cpus()/32);
	pr_info("%d\n",size);
	ptr = kmalloc(size, GFP_KERNEL);
	if (!ptr)
		pr_info("Error allocating memory\n");
	file_open=filp->private_data;
	file_open->private=ptr;
	kfree(ptr);
	return 0;
}

static int stat_seq_release(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t stat_seq_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    return count;
}

static ssize_t stat_seq_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	u64 __user *out = (u64 __user *)buf;
	u64 pcount;
	struct page *ppages=NULL;
	long src=*f_pos;
	long pfn;

	pfn=src/KPMSIZE;
	count=min_t(size_t,count, (max_pfn*KPMSIZE)-src);
	if(src & KPMSIZE || count & KPMSIZE)
		return -EINVAL;
	while(count>0){
		if(pfn_valid(pfn)){
			ppages=pfn_to_page(pfn);
		}
		pfn++;
		if(!ppages){
			pcount=0;
		}
		else{
			pcount=page_mapcount(ppages);
		}
		if(put_user(pcount,out++)){
			return -EFAULT;
		}
		count=count-KPMSIZE;
	}
	*f_pos += (char __user *)out - buf;

    return 0;
}

static struct file_operations seq_fops = {
        .owner =    THIS_MODULE,
        .open =     stat_seq_open,
        .release =  stat_seq_release,
        .write =    stat_seq_write,
        .read =     stat_seq_read
};

static int nodo_init(void){
	int retval;
	int major;
	dev_t my_device;
	int i=0;
	if(dynamic){
		pr_info("Antes del chrdev_regiose\n");
		retval= alloc_chrdev_region(&device_number,0,minor,"embedded");
		if(retval<0){
			pr_info("Fallo alloc_chrdev_region\n");
		}
	}else{
		device_number= MKDEV(180,0);
		pr_info("Antes del register_chrdev_region\n");
		retval= register_chrdev_region(device_number,minor,"embedded");
		if(retval<0){
			pr_info("Fallo register_chrdev_region\n");
		}
	}
	if(!retval){
		major=MAJOR(device_number);
		pr_info("Antes del class_create\n");
		my_class=class_create(THIS_MODULE, "nodoIsma");

		if(IS_ERR(my_class)){
			pr_info("Fallo create\n");
		}
		for(i=0;i<minor;i++){
			my_device=MKDEV(major,i);
			cdev_init(&my_cdev[i],&seq_fops);
			retval=cdev_add(&my_cdev[i],my_device,1);
			if(retval<0){
			pr_info("Fallo cdev_add\n");
			}
			else{
				pr_info("Antes del device Create\n");
				device_create(my_class,NULL, my_device,NULL, "seq%d",i);
			}
		}
	}else{
		pr_err("%s: Failed in allocating device device_number " "Error:%d\n", __func__,retval);
	}
	return retval;
}



static void nodo_exit(void){
	int i;
	int major=MAJOR(device_number);
	dev_t my_device;
	for (i=0;i<minor;i++){
		my_device=MKDEV(major,i);
		pr_info("Antes del cdev_del\n");
		cdev_del(&my_cdev[i]);
		pr_info("Antes del devicce_destroy\n");
		device_destroy(my_class,my_device);
	}
	pr_info("Antes del class_destroy\n");
	class_destroy(my_class);
	pr_info("Antes del unregister_chrdev_region\n");
	unregister_chrdev_region(device_number,minor);
	pr_info("%s: In exit\n", __func__);
}

module_init(nodo_init);
module_exit(nodo_exit);