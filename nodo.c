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
#define BUF_LEN 2048
#define KPMSIZE sizeof(u64)

static int minor = MENOR;

module_param(minor, int, S_IRUGO);


bool dynamic =true;
dev_t device_number;
struct class *my_class;
struct cdev my_cdev[MENOR];


static int stat_seq_open(struct inode *inode, struct file *filp) {
	char *buf;
	pr_info("antes del kmalloc cuando cambio");
	buf = kmalloc(BUF_LEN*sizeof(char), GFP_KERNEL);
	pr_info("Deberiamos ver algo aqui :%p\n",&buf);
	if (!buf){
		pr_info("Error allocating memory\n");
		return -ENOMEM;
	}else{
		buf[0]='h';
		buf[1]='o';
		buf[2]='l';
		buf[3]='a';
		buf[4]='\n';
	}
	pr_info("antes de colocar el puntero\n");
	filp->private_data=buf;
	return 0;
	//struct file *file_open;
    //unsigned size=4096*(1+num_possible_cpus()/32);
	//pr_info("%d\n",size);
	/*pr_info("antes de meter private_data\n");
	file_open=filp->private_data;*/
	/*pr_info("antes de colocar el puntero:buf\n");
	file_open->buf=buf;
	pr_info("antes de colocar el size\n");
	file_open->size=BUF_LEN;*/
	/*pr_info("antes de colocar el puntero\n");
	file_open->private=ptr;
	pr_info("Antes del kfree\n");*/
	
}

static int stat_seq_release(struct inode *inode, struct file *filp) {
	pr_info("Antes del kfree\n");
	kfree(filp->private_data);
	pr_info("Finalizo el release\n");
    return 0;
}

static ssize_t stat_seq_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    return count;
}

static ssize_t stat_seq_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	char *origen=filp->private_data;
	char *destino=buf;
	size_t i=0;
	size_t pos=*f_pos;
	for(;i<count;i++){
		char *dir=origen+pos+i;
		pr_info("dir=%p && i=%ld && pos=%ld && origen=%p && destino=%p\n",dir, i, pos,origen,destino+i);
		if(put_user(*dir,destino+i))
			return -EFAULT;	
		char c;	
		if(get_user(c,destino+i))
			pr_info("puto error de los cojons\n");
		pr_info("linea=%c\n",c);
	}
	*f_pos+=i;
	return i;
	/*long pfn;
	u64 __user *out = (u64 __user *)buf;
	u64 pcount;
	struct page *ppages;
	long src=*f_pos;
	pr_info("entramos en el read\n");
	pr_info("%ld\n",src);
	pr_info("calculamos tam pagina\n");
	pfn=src/KPMSIZE;
	pr_info("%ld\n",pfn);
	//count=min_t(size_t,count, (max_pfn * KPMSIZE) - src);
	if(src & KPMSIZE || count & KPMSIZE){
		pr_info("Error -EINVAL\n");
		return -EINVAL;

	}
	while(count>0){
		/*if(pfn_valid(pfn)){
			pr_info("Antes del paginado\n");
			ppages=pfn_to_page(pfn);
		}
		pfn++;
		if(!ppages){
			pcount=0;
		}
		else{
			//pr_info("Antes del mapeado de las paginas\n");
			pcount=page_mapcount(ppages);
		}
		if(put_user(pcount,out++)){
			return -EFAULT;
		}
		count=count-KPMSIZE;
	}
	*f_pos += (char __user *)out - buf;*/
    
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