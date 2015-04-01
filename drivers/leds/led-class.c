/*
 * LED Class Core
 *
 * Copyright (C) 2005 John Lenz <lenz@cs.wisc.edu>
 * Copyright (C) 2005-2007 Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include "leds.h"

#define LED_BUFF_SIZE 50

#ifdef CONFIG_PANTECH_LED_BLINK_CONTROL
/* -------------------------------------------------------------------- */
/*   debug option - p11309 */
/* -------------------------------------------------------------------- */
#define DBG_ENABLE

#ifdef DBG_ENABLE
#define dbg(fmt, args...)   pr_debug("[LED] " fmt, ##args)
#else
#define dbg(fmt, args...)
#endif
#define dbg_func_in()       dbg("[+] %s\n", __func__)
#define dbg_func_out()      dbg("[-] %s\n", __func__)
#define dbg_line()          dbg("[LINE] %d(%s)\n", __LINE__, __func__)

/* -------------------------------------------------------------------- */
#endif /* CONFIG_PANTECH_LED_BLINK_CONTROL */

static struct class *leds_class;

static void led_update_brightness(struct led_classdev *led_cdev)
{
	if (led_cdev->brightness_get)
		led_cdev->brightness = led_cdev->brightness_get(led_cdev);
}

static ssize_t led_brightness_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	led_update_brightness(led_cdev);

	return snprintf(buf, LED_BUFF_SIZE, "%u\n", led_cdev->brightness);
}

static ssize_t led_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;

		if (state == LED_OFF)
			led_trigger_remove(led_cdev);
		led_set_brightness(led_cdev, state);
	}

	return ret;
}

static ssize_t led_max_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	unsigned long state = 0;

	ret = strict_strtoul(buf, 10, &state);
	if (!ret) {
		ret = size;
		if (state > LED_FULL)
			state = LED_FULL;
		led_cdev->max_brightness = state;
		led_set_brightness(led_cdev, led_cdev->brightness);
	}

	return ret;
}

static ssize_t led_max_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	return snprintf(buf, LED_BUFF_SIZE, "%u\n", led_cdev->max_brightness);
}

#ifdef CONFIG_PANTECH_LED_BLINK_CONTROL
static ssize_t led_blink_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	dbg("brightness show: name=%s, brightness=%d,  onMS=%lu,  offMS=%lu \n",
		led_cdev->name, led_cdev->brightness, led_cdev->blink_delay_on, led_cdev->blink_delay_off);


	return snprintf(buf, LED_BUFF_SIZE, "%u\n", led_cdev->brightness);
}

static ssize_t led_blink_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long data;	//32bit 31-15;Off Time, 15-7; 0n Time, 7-3;brightness,3-1;reserved , 0;enable mask

	int flag;//0xf= enable, 0x0=disable
	int on;  // 1 unit =10 msec
	int off; //	1 unit =10 msec
	int level; //led brightness level

	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;

//	ret = strict_strtoul(buf, 10, &data);
	ret = kstrtoul(buf, 10, &data);


	if (!ret){
		ret= size;

		//check bit mask
		flag = data & 0x1;
		on = ((data >> 8) & 0xFF)*10;
		off = ((data >> 16) & 0xFFFF)*10;
		level = ((data >>3)) & 0xF;
		if (level > 10){
			printk("%s: name=%s, input level=%d, It will be changed to level 10. \n",
						__func__,led_cdev->name, level);
			level=10;// max level is 10
		}
		//set parameter
		led_cdev->blink_delay_on = on;
		led_cdev->blink_delay_off = off;
		led_cdev->brightness= level<<4;

		if (flag == 1 && led_cdev->brightness > 0) {
		//printk("%s: name=%s, level=%d, brightness=%d  ,onMS=%lu,  offMS=%lu , flag=%d\n",
		//	__func__,led_cdev->name, level,led_cdev->brightness, led_cdev->blink_delay_on,led_cdev->blink_delay_off, flag);

		led_cdev->blink_set(
					led_cdev,
					&led_cdev->blink_delay_on,
					&led_cdev->blink_delay_off,
					(led_cdev->brightness*100)/led_cdev->max_brightness);

		}
		else{
			led_brightness_set(led_cdev, 0);
		//	printk("%s: name=%s, level=%d, brightness=%d  ,onMS=%lu,  offMS=%lu , flag=%d\n",
		//	__func__,led_cdev->name, level,led_cdev->brightness, led_cdev->blink_delay_on,led_cdev->blink_delay_off, flag);
		}

	}
	return ret;

}
#endif /* CONFIG_PANTECH_LED_BLINK_CONTROL */

static struct device_attribute led_class_attrs[] = {
	__ATTR(brightness, 0644, led_brightness_show, led_brightness_store),
	__ATTR(max_brightness, 0644, led_max_brightness_show,
			led_max_brightness_store),
#ifdef CONFIG_PANTECH_LED_BLINK_CONTROL
	__ATTR(pan_led, 0664, led_blink_enable_show,
			led_blink_enable_store),
#endif /* CONFIG_PANTECH_LED_BLINK_CONTROL */
#ifdef CONFIG_LEDS_TRIGGERS
	__ATTR(trigger, 0644, led_trigger_show, led_trigger_store),
#endif
	__ATTR_NULL,
};

static void led_timer_function(unsigned long data)
{
	struct led_classdev *led_cdev = (void *)data;
	unsigned long brightness;
	unsigned long delay;

	if (!led_cdev->blink_delay_on || !led_cdev->blink_delay_off) {
		led_set_brightness(led_cdev, LED_OFF);
		return;
	}

	brightness = led_get_brightness(led_cdev);
	if (!brightness) {
		/* Time to switch the LED on. */
		brightness = led_cdev->blink_brightness;
		delay = led_cdev->blink_delay_on;
	} else {
		/* Store the current brightness value to be able
		 * to restore it when the delay_off period is over.
		 */
		led_cdev->blink_brightness = brightness;
		brightness = LED_OFF;
		delay = led_cdev->blink_delay_off;
	}

	led_set_brightness(led_cdev, brightness);

	mod_timer(&led_cdev->blink_timer, jiffies + msecs_to_jiffies(delay));
}

/**
 * led_classdev_suspend - suspend an led_classdev.
 * @led_cdev: the led_classdev to suspend.
 */
void led_classdev_suspend(struct led_classdev *led_cdev)
{
	led_cdev->flags |= LED_SUSPENDED;
	led_cdev->brightness_set(led_cdev, 0);
}
EXPORT_SYMBOL_GPL(led_classdev_suspend);

/**
 * led_classdev_resume - resume an led_classdev.
 * @led_cdev: the led_classdev to resume.
 */
void led_classdev_resume(struct led_classdev *led_cdev)
{
	led_cdev->brightness_set(led_cdev, led_cdev->brightness);
	led_cdev->flags &= ~LED_SUSPENDED;
}
EXPORT_SYMBOL_GPL(led_classdev_resume);

static int led_suspend(struct device *dev, pm_message_t state)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_suspend(led_cdev);

	return 0;
}

static int led_resume(struct device *dev)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_resume(led_cdev);

	return 0;
}

/**
 * led_classdev_register - register a new object of led_classdev class.
 * @parent: The device to register.
 * @led_cdev: the led_classdev structure for this device.
 */
int led_classdev_register(struct device *parent, struct led_classdev *led_cdev)
{
	led_cdev->dev = device_create(leds_class, parent, 0, led_cdev,
				      "%s", led_cdev->name);
	if (IS_ERR(led_cdev->dev))
		return PTR_ERR(led_cdev->dev);

#ifdef CONFIG_LEDS_TRIGGERS
	init_rwsem(&led_cdev->trigger_lock);
#endif
	/* add to the list of leds */
	down_write(&leds_list_lock);
	list_add_tail(&led_cdev->node, &leds_list);
	up_write(&leds_list_lock);

	if (!led_cdev->max_brightness)
		led_cdev->max_brightness = LED_FULL;

	led_update_brightness(led_cdev);

	init_timer(&led_cdev->blink_timer);
	led_cdev->blink_timer.function = led_timer_function;
	led_cdev->blink_timer.data = (unsigned long)led_cdev;

#ifdef CONFIG_LEDS_TRIGGERS
	led_trigger_set_default(led_cdev);
#endif

	printk(KERN_DEBUG "Registered led device: %s\n",
			led_cdev->name);

	return 0;
}
EXPORT_SYMBOL_GPL(led_classdev_register);

/**
 * led_classdev_unregister - unregisters a object of led_properties class.
 * @led_cdev: the led device to unregister
 *
 * Unregisters a previously registered via led_classdev_register object.
 */
void led_classdev_unregister(struct led_classdev *led_cdev)
{
#ifdef CONFIG_LEDS_TRIGGERS
	down_write(&led_cdev->trigger_lock);
	if (led_cdev->trigger)
		led_trigger_set(led_cdev, NULL);
	up_write(&led_cdev->trigger_lock);
#endif

	/* Stop blinking */
	led_brightness_set(led_cdev, LED_OFF);

	device_unregister(led_cdev->dev);

	down_write(&leds_list_lock);
	list_del(&led_cdev->node);
	up_write(&leds_list_lock);
}
EXPORT_SYMBOL_GPL(led_classdev_unregister);

static int __init leds_init(void)
{
	leds_class = class_create(THIS_MODULE, "leds");
	if (IS_ERR(leds_class))
		return PTR_ERR(leds_class);
	leds_class->suspend = led_suspend;
	leds_class->resume = led_resume;
	leds_class->dev_attrs = led_class_attrs;
	return 0;
}

static void __exit leds_exit(void)
{
	class_destroy(leds_class);
}

subsys_initcall(leds_init);
module_exit(leds_exit);

MODULE_AUTHOR("John Lenz, Richard Purdie");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Class Interface");
