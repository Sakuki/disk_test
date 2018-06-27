#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#define COEVDEMO_MAJOR 255 	//预设cdevdemo的主设备号

  
#ifndef MEMDEV_SIZE  
#define MEMDEV_SIZE 4096  
#endif  
 

static int cdevdemo_major = COEVDEMO_MAJOR;
char temp[256]={0};

struct cdevdemo_dev
{
	struct cdev cdev;
};

struct cdevdemo_dev *cdevdemo_devp;	//设备结构体指针

int cdevdemo_open(struct inode *inode, struct file *filp)
{
//	printk(KERN_NOTICE "=======cdevdemo_open");
	return 0;
}

int cdevdemo_release(struct inode *inode,struct file *filp)
{
//	printk(KERN_NOTICE "=======cdevdemo_release");
	return 0;
}

static ssize_t cdevdemo_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	const char data[]="This is a read test demo\n ";
	if(copy_to_user(buf,data,strlen(data)))
	{
		return -EFAULT;
	}
	printk(KERN_NOTICE "a read test\n");
	return len;
}

static ssize_t cdevdemo_write(struct file *filep, const char __user *buf, size_t len, loff_t *ppos)
{
	if(copy_from_user(temp,buf,len))
	{
		return -EFAULT;
	}
	printk(KERN_NOTICE "write %s\n",temp);
	return len;
}

loff_t cdevdemo_llseek(struct file *filp, loff_t off, int whence)
{
//	struct cdevdemo_dev *dev = filp->private_data;
	loff_t newpos;
	switch(whence){
		case 0:		//SEEK_SET文件起始位置
			newpos = off;
			break;
		case 1:
			newpos = filp->f_pos+off;		//SEEK_CUR当前位置
			break;
		case 2:
			newpos = MEMDEV_SIZE -1+off;
		//	newpos = 254+off;		//SEEK_END文件结尾
			break;
		default:
			return -EINVAL;
	}
	if(newpos < 0)
		return -EINVAL;
	filp->f_pos=newpos;
	return newpos;
}

//文件操作结构体
static const struct file_operations cdevdemo_fops =
{
	.owner = THIS_MODULE,
	.open = cdevdemo_open,
	.release = cdevdemo_release,
	.read = cdevdemo_read,
	.write = cdevdemo_write,
	.llseek = cdevdemo_llseek,
};

//初始化并注册cdev
static void cdevdemo_setup_cdev(struct cdevdemo_dev *dev, int index)
{
	int err, devno = MKDEV(cdevdemo_major, index);
	
	//初始化一个字符设备，设备所支持的操作在cdevdemo_fops中
	cdev_init(&dev->cdev, &cdevdemo_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &cdevdemo_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
	{
		printk(KERN_NOTICE "Error %d add cdevdemo %d", err, index);
	}
} 

int cdevdemo_init(void)
{
	int ret;
	dev_t devno = MKDEV(cdevdemo_major, 0);
	
	struct class *cdevdemo_class;
	//申请设备号，如果失败就采用动态申请方式
	if(cdevdemo_major)
	{
		ret = register_chrdev_region(devno, 1, "cdevdemo");
	}else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, "cdevdemo");
		cdevdemo_major = MAJOR(devno);
	}
	if(ret < 0)
	{
		return ret;
	}
	//动态申请设备结构体内存
	cdevdemo_devp = kmalloc( sizeof(struct cdevdemo_dev), GFP_KERNEL );
	if(!cdevdemo_devp)
	{
		ret = -ENOMEM;
		printk(KERN_NOTICE "Error add cdevdemo");
		goto fail_malloc;
	}
	
	memset(cdevdemo_devp, 0, sizeof(struct cdevdemo_dev));
	cdevdemo_setup_cdev(cdevdemo_devp, 0);
	/*下面两行是创建了一个总线类型，会在/sys/class下生成cdevdemo目录
	还有一个重要作用是执行device_create后会在/dev/下自动生成cdevdemo设备节点。
	而如果不调用此函数，如果想通过设备节点访问设备，则需要手动mknod来创建设备节点后再访问。
	*/
	cdevdemo_class = class_create(THIS_MODULE, "cdevdemo");
	device_create(cdevdemo_class, NULL, MKDEV(cdevdemo_major, 0), NULL, "cdevdemo");
	
	printk(KERN_NOTICE "=========cdevdemo_init success");
	
	return 0;
	
	fail_malloc:
		unregister_chrdev_region(devno, 1);
} 

void cdevdemo_exit(void)	//模块卸载
{
	printk(KERN_NOTICE "End cdevdemo");
	cdev_del(&cdevdemo_devp->cdev);		//注销cdev
	kfree(cdevdemo_devp);				//释放设备结构体内存
	unregister_chrdev_region(MKDEV(cdevdemo_major, 0), 1);		//释放设备号
}

MODULE_LICENSE("Dual BSD/GPL");
module_param(cdevdemo_major, int, S_IRUGO);
module_init(cdevdemo_init);
module_exit(cdevdemo_exit);
