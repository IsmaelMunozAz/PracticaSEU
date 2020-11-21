#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define MAX_DEVICES 5

bool dynamic =true;
dev_t device_number;
struct class *my_class;
static struct cdev my_cdev[MAX_DEVICES];


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
		retval= alloc_chrdev_region(&device_number,0,MAX_DEVICES,"embedded");
	}else{
		device_number= MKDEV(180,0);
		retval= register_chdev_region(device_number,MAX_DEVICES,"embedded");
	}
	if(!retval){
		major=MAJOR(device_number);
		my_class=class_create(THIS_MODULE, "my_driver_class");
		for(i=0;i<MAX_DEVICES;i++){
			my_device=MKDEV(major,i);
			cdev_init(&my_cdev[i]);
			retval=cdev_add(&my_cdev[i],my_device,1);
			if(retval){
				pr_info("%s: Failed in adding cdev to subsystem " "retval:%d\n", __func__,retval);

			}else{
				device_create(my_class,NULL, my_device,NULL, "my_null%d",1);
			}
		}
	}else{
		pr_err("%s: Failed in allocating device device_number " "Error:%d\n", __func__,retval);
	}
	return retval;
}



static void nodo_exit(void){

}

