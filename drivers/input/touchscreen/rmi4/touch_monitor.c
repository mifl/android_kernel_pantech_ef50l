/* 
 * Touch Monitor Interface 
 * Ver 0.1
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>   

#include "touch_monitor.h"
#include "touch_monitor/touch_cmd.h"

#include "touch_fops.h"
#include "pantech_cfg.h"

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#define X_SENSOR_NUM 13
#define Y_SENSOR_NUM 23

extern int ioctl_diag_debug(unsigned long arg);
extern int ImageBuffer_int_temp[13*23];

static long monitor_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;    
	
	int return_to_user = 0;
    
	switch (cmd) 
	{
		case TOUCH_IOCTL_SENSOR_X:
			return_to_user = X_SENSOR_NUM;
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._X\n", __func__);
			break;
		case TOUCH_IOCTL_SENSOR_Y:
			return_to_user = Y_SENSOR_NUM;
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._Y\n", __func__);
			break;
		case TOUCH_IOCTL_CHECK_BASE:
			send_reference_data_for_self(arg);
			break;
		case TOUCH_IOCTL_SELF_TEST:
			return_to_user = panel_test(X_SENSOR_NUM * Y_SENSOR_NUM);
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops..\n", __func__); 
			break;
		case TOUCH_IOCTL_CHARGER_MODE:
			break;
		case TOUCH_IOCTL_GET_CHARGER_MODE:
			//return_to_user = charger_mode /*prev_charger_data*/;
			//if (copy_to_user(argp, &return_to_user, sizeof(int)))
			//	pr_err("%s: Ops.._Charger\n", __func__);	
			break;
		case DIAG_DEBUG:
			return_to_user = ioctl_diag_debug(arg);
			//printk("[TOUCH_MONITOR]call DIAG_DEBUG\n");				
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				//pr_err("%s: Ops.._diag\n", __func__);
 			break;
		case GET_REFERENCE_DATA:
/*			
			return_to_user = send_reference_data(arg);
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._reference\n", __func__);
*/
    			if (copy_to_user(argp, &ImageBuffer_int_temp, 13 * 23 * sizeof(int)))
					pr_err("%s: Ops.._GET_REFERENCE_DATA\n", __func__);
/*					
  	  		if (copy_to_user(argp, &reference_data, sizeof(reference_data))){
  	    			pr_err("[TOUCH IO] ATMEL_GET_REFERENCE_DATA is failed\n");
  	  		}
*/			
			break;
		case TOUCH_IOCTL_GET_TOUCH_ERR:
			break;
		case TOUCH_IOCTL_SET_VKEYBOARD:
			break;
		default:
			break;
    }
    return return_to_user;
}
static int monitor_open(struct inode *inode, struct file *file) 
{
    return 0; 
}

static ssize_t monitor_read(struct file *file, char *buf, size_t count, loff_t *ppos) 
{
    return 0; 
}

static ssize_t monitor_write(struct file *file, const char *buf, size_t count, loff_t *ppos) 
{
    int nBufSize=0;
    if((size_t)(*ppos) > 0) return 0;
    if(buf!=NULL)
    {
        nBufSize=strlen(buf);
        if(strncmp(buf, "queue", 5)==0)
        {
        }
        if(strncmp(buf, "debug", 5)==0)
        {			
        }
        if(strncmp(buf, "debugoff", 8)==0)
        {			
        }
        if(strncmp(buf, "checkcal", 8)==0)
        {			
        }
        if(strncmp(buf, "cal", 3)==0)
        {			
        }
        if(strncmp(buf, "save", 4)==0)
        {			
        }
        if(strncmp(buf, "reset1", 6)==0)
        {	
        }
        if(strncmp(buf, "reset2", 6)==0)
        {	
        }
        if(strncmp(buf, "reset3", 6)==0)
        {	
        }
        if(strncmp(buf, "reference", 9)==0)
        {
        }
        if(strncmp(buf, "suspend", 5)==0)
        {
        }
        if(strncmp(buf, "resume",6)==0)
        {
        }
    }
    *ppos +=nBufSize;
    return nBufSize;
}

static int monitor_release(struct inode *inode, struct file *file) 
{
    //todo
    return 0; 
}

static struct file_operations monitor_fops = 
{
	.owner =    THIS_MODULE,
	.unlocked_ioctl =    monitor_ioctl,  // mirinae
	.read =     monitor_read,
	.write =    monitor_write,
	.open =     monitor_open,
	.release =  monitor_release,
};

static struct miscdevice touch_monitor = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "touch_monitor",
	.fops =     &monitor_fops,
};

// call in driver init function
void touch_monitor_init(void) {
	int rc;
	rc = misc_register(&touch_monitor);
	if (rc) {
		pr_err("::::::::: can''t register touch_monitor\n");
	}
	//init_proc();
}

// call in driver remove function
void touch_monitor_exit(void) {
	misc_deregister(&touch_monitor);
	//remove_proc();
}
