/*
 * Core Source for:
 * 
 *
 * Copyright (C) 2012 Pantech, Inc.
 * 
 */

/* -------------------------------------------------------------------- */
/* for SkyTestMenu */
/* -------------------------------------------------------------------- */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>   

#include "touch_monitor/touch_cmd.h"

#include "touch_fops.h"
#include "pantech_cfg.h"

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/delay.h>

struct rmi_fn_54_data *data;

#define X_SENSOR_NUM 13
#define Y_SENSOR_NUM 23

extern void rmi_chargermode(int flag);
extern int send_reference_data(unsigned long arg);
extern int panel_test(int count);
extern int prev_charger_data;
extern int touch_error_check_flag;

extern void rmi_clear_finger(struct input_dev *input);
extern struct input_dev *pantech_dev;
extern void rmi_VirtualKeyboardmode(int flag);

extern int ghost_touch_check;
extern int pantech_touch_debug;

int ioctl_diag_debug(unsigned long arg) 
{
    /*
     * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
     * Returns diag result when the arg. is in range of 0~223. 
     */
    int ret_value = 1;
   	//int retval = 1;
   	rmi_scan_function();
    
    if (arg == TOUCH_IOCTL_DIAG_DEBUG_DELTA) 
    {	
		rmi_delta();
    }
    else if (arg == TOUCH_IOCTL_DIAG_DEBUG_REF) 
    {
		//retval = rmi_raw_capacitance();
		rmi_baseline(X_SENSOR_NUM * Y_SENSOR_NUM);
    }
    else if (arg == TOUCH_IOCTL_DIAG_DEBUG_BASELINE) 
    {
		rmi_baseline(X_SENSOR_NUM * Y_SENSOR_NUM);
    }
    else if (arg == TOUCH_IOCTL_DIAG_DEBUG_OPERATEMODE) 
    {

    }
    else if (arg > 224-1)
    {

    }

    return ret_value;
}
extern int charger_mode;
int call_mode = 4;
static long touch_fops_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
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
			rmi_chargermode(arg);
            //printk("[Touch]charger_mode = %lu\n", arg);
            if (arg == 3 || arg == 4)
                call_mode = arg;
			break;
		case TOUCH_IOCTL_GET_CHARGER_MODE:
			return_to_user = charger_mode /*prev_charger_data*/;
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._Charger\n", __func__);	
			break;
		case DIAG_DEBUG:
			return_to_user = ioctl_diag_debug(arg);
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._diag\n", __func__);
 			break;
		case GET_REFERENCE_DATA:
			return_to_user = send_reference_data(arg);
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Ops.._reference\n", __func__);
			break;
		case TOUCH_IOCTL_GET_TOUCH_ERR:
			return_to_user = touch_error_check_flag;
			if (copy_to_user(argp, &return_to_user, sizeof(int)))
				pr_err("%s: Touch IC error detection\n", __func__);
			break;
		case TOUCH_IOCTL_SET_VKEYBOARD:
			//printk("[TOUCH]touch_fops = %lu\n", arg);
			rmi_VirtualKeyboardmode(arg);
			break;
		default:
			break;
    }
    return return_to_user;
}

static ssize_t touch_fops_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    int nBufSize=0;
	
    if((size_t)(*ppos) > 0) return 0;

	if(buf!=NULL) {
		nBufSize=strlen(buf);

		if(strncmp(buf, "debug", 5)==0){
			printk("Touch debug enable!!\n");
			pantech_touch_debug =true;
	        }
	        if(strncmp(buf, "debugoff", 8)==0){
			printk("Touch debug disable!!\n");
			pantech_touch_debug =false;
	        }
		if(strncmp(buf, "reset", 5)==0) {
			gpio_set_value(TOUCH_RST_GPIO, 0);

			rmi_clear_finger(pantech_dev);
			
			msleep(50);
			gpio_set_value(TOUCH_RST_GPIO, 1);	
	        }
	        if(strncmp(buf, "info", 4)==0) {
			printk("Ghost touch count : %d\n", ghost_touch_check);
	        }
	        if(strncmp(buf, "d2", 2)==0) {

	        }
	        if(strncmp(buf, "d3", 2)==0) {

	        }
	        if(strncmp(buf, "cal", 3)==0) {

	        }
    }
    *ppos +=nBufSize;
    return nBufSize;
}

static struct file_operations touch_fops = {
    .owner = THIS_MODULE,
#if ((LINUX_VERSION_CODE & 0xFFFF00) < KERNEL_VERSION(3,0,0))
   .ioctl = touch_fops_ioctl,
#else
    .unlocked_ioctl = touch_fops_ioctl,
#endif
    .write = touch_fops_write,
};

static struct miscdevice touch_fops_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "touch_fops",
    .fops = &touch_fops,
};

// call in driver init function

void touch_fops_init(void) {
	int rc;
	rc = misc_register(&touch_fops_dev);
	if (rc) {
		pr_err("::::::::: can''t register touch_fops\n");
	}
}
// call in driver remove function
void touch_fops_exit(void) {
	misc_deregister(&touch_fops_dev);
}

