#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <linux/device.h>

MODULE_LICENSE("Dual BSD/GPL");

#define MENOR 1

static int minor = MENOR;

module_param(minor, int, S_IRUGO);


bool dynamic =true;
dev_t device_number;
struct class *my_class;
struct cdev my_cdev[MENOR];



static int seq_open(struct inode *inode, struct file *filp) {
    return 0;
}

static int seq_release(struct inode *inode, struct file *filp) {
    return 0;
}

static ssize_t seq_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    return count;
}

static ssize_t seq_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    return 0;
}

static struct file_operations seq_fops = {
        .owner =    THIS_MODULE,
        .open =     seq_open,
        .release =  seq_release,
        .write =    seq_write,
        .read =     seq_read
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