/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *         Benoit Goby <benoit@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>

#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/usb/android.h>

#include "gadget_chips.h"
#ifndef DEBUG
#define DEBUG 1
#endif
/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

#include "f_diag.c"
#include "f_qdss.c"
#include "f_rmnet_smd.c"
#include "f_rmnet_sdio.c"
#include "f_rmnet_smd_sdio.c"
#include "f_rmnet.c"
#include "f_audio_source.c"
#include "f_mass_storage.c"
#include "u_serial.c"
#include "u_sdio.c"
#include "u_smd.c"
#include "u_bam.c"
#include "u_rmnet_ctrl_smd.c"
#include "u_ctrl_hsic.c"
#include "u_data_hsic.c"
#include "u_ctrl_hsuart.c"
#include "u_data_hsuart.c"
#include "f_serial.c"
#include "f_acm.c"
#include "f_adb.c"
#include "f_ccid.c"
#include "f_mtp.c"
#include "f_accessory.c"
#ifdef CONFIG_USB_ANDROID_CDC_ECM
#include "f_ecm.c"
#else
#define USB_ETH_RNDIS y
#include "f_rndis.c"
#include "rndis.c"
#endif
#include "u_ether.c"
#include "u_bam_data.c"
#include "f_mbim.c"
#include "f_qc_ecm.c"
#include "f_qc_rndis.c"
#include "u_qc_ether.c"
#ifdef CONFIG_TARGET_CORE
#include "f_tcm.c"
#endif
#ifdef CONFIG_PANTECH_ANDROID_OBEX
#include "pantech_f_obex.c"
#endif

#ifdef CONFIG_PANTECH_ANDROID_OTG
#include <linux/mfd/pm8xxx/misc.h>
#endif /* CONFIG_PANTECH_ANDROID_OTG */

#define FEATURE_PANTECH_MODS
#define FEATURE_PANTECH_MS_OS_COMPATIBLE
#define FEATURE_PANTECH_USB_FIELDTEST_WORKAROUND

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by userspace */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001

#define ANDROID_DEVICE_NODE_NAME_LENGTH 11
#ifdef FEATURE_PANTECH_MS_OS_COMPATIBLE
/* Microsoft Extended Configuration Descriptor Header Section */
struct ms_ext_config_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u8	bCount;
	__u8	reserved[7];
};

/* Microsoft Extended Configuration Descriptor Function Section */
struct ms_ext_config_desc_function {
	__u8	bFirstInterfaceNumber;
	__u8	bInterfaceCount;
	__u8	compatibleID[8];
	__u8	subCompatibleID[8];
	__u8	reserved[6];
};

struct ms_ext_config_desc{
	struct ms_ext_config_desc_header header;
	struct ms_ext_config_desc_function function[9];/* max interface */
};

struct pantech_ext_config_desc{
	u32 type;
	struct ms_ext_config_desc desc;
};

static struct pantech_ext_config_desc  *p_pantech_ext_config_desc;

static struct pantech_ext_config_desc  pantech_ext_config_desc_list[] =
{
	{
		/*"mtp" : 0*/
		0,
		{
			{
				.dwLength = __constant_cpu_to_le32(16 + 24),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(1), /* mtp only */
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
					.compatibleID = { 'M', 'T', 'P' },
				},
			}
		}
	},
	{
		/*"ptp" : 1*/
		1,
    {
			{
				.dwLength = __constant_cpu_to_le32(16 + 24),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(1),
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
					.compatibleID = { 'P', 'T', 'P' },
				},
			}
		}
	},
	{
		/*"mtp,adb" : 2*/
		2,
    {
			{
				.dwLength = __constant_cpu_to_le32(16 + 24*2),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(2),/* check */
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
					.compatibleID = { 'M', 'T', 'P' },
				},
				{
					.bFirstInterfaceNumber = 1,
					.bInterfaceCount = 1,
				},
			}
		}
	}
	,
	{
		/*"ptp,adb" : 3*/
		3,
    {
			{
				.dwLength = __constant_cpu_to_le32( 16 + 24*2),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(2),
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
					.compatibleID = { 'P', 'T', 'P' },
				},
				{
					.bFirstInterfaceNumber = 1,
					.bInterfaceCount = 1,
				},
			}
		}
	}
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
	,
	{
		/*"cdrom,mtp,diag,serial,obex" : 4 */
		4,
    {
			{
				.dwLength = __constant_cpu_to_le32( 16 + 24*8),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(8),
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 1,
					.bInterfaceCount = 1,
					.compatibleID = { 'M', 'T', 'P' },
				},
				{
					.bFirstInterfaceNumber = 2,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 3,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 4,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 5,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 6,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 7,
					.bInterfaceCount = 1,
				},
			}
		}
	}
	,
	{
		/*"cdrom,mtp,diag,serial,obex,adb" : 5 */
		5,
    {
			{
				.dwLength = __constant_cpu_to_le32( 16 + 24*9),
				.bcdVersion = __constant_cpu_to_le16(0x0100),
				.wIndex = __constant_cpu_to_le16(4),
				.bCount = __constant_cpu_to_le16(9),
			},
			{
				{
					.bFirstInterfaceNumber = 0,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 1,
					.bInterfaceCount = 1,
					.compatibleID = { 'M', 'T', 'P' },
				},
				{
					.bFirstInterfaceNumber = 2,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 3,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 4,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 5,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 6,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 7,
					.bInterfaceCount = 1,
				},
				{
					.bFirstInterfaceNumber = 8,
					.bInterfaceCount = 1,
				},
			}
		}
	}
#endif
};
#endif

struct android_usb_function {
	char *name;
	void *config;

	struct device *dev;
	char *dev_name;
	struct device_attribute **attributes;

	/* for android_conf.enabled_functions */
	struct list_head enabled_list;

	struct android_dev *android_dev;

	/* Optional: initialization during gadget bind */
	int (*init)(struct android_usb_function *, struct usb_composite_dev *);
	/* Optional: cleanup during gadget unbind */
	void (*cleanup)(struct android_usb_function *);
	/* Optional: called when the function is added the list of
	 *		enabled functions */
	void (*enable)(struct android_usb_function *);
	/* Optional: called when it is removed */
	void (*disable)(struct android_usb_function *);

	int (*bind_config)(struct android_usb_function *,
			   struct usb_configuration *);

	/* Optional: called when the configuration is removed */
	void (*unbind_config)(struct android_usb_function *,
			      struct usb_configuration *);
	/* Optional: handle ctrl requests before the device is configured */
	int (*ctrlrequest)(struct android_usb_function *,
					struct usb_composite_dev *,
					const struct usb_ctrlrequest *);
};

struct android_dev {
	const char *name;
	struct android_usb_function **functions;
	struct usb_composite_dev *cdev;
	struct device *dev;

	bool enabled;
	int disable_depth;
	struct mutex mutex;
	struct android_usb_platform_data *pdata;

	bool connected;
	bool sw_connected;
	char pm_qos[5];
	struct pm_qos_request pm_qos_req_dma;
	struct work_struct work;

	/* A list of struct android_configuration */
	struct list_head configs;
	int configs_num;

	/* A list node inside the android_dev_list */
	struct list_head list_item;

};

struct android_configuration {
	struct usb_configuration usb_config;

	/* A list of the functions supported by this config */
	struct list_head enabled_functions;

	/* A list node inside the struct android_dev.configs list */
	struct list_head list_item;
};

static struct class *android_class;
static struct list_head android_dev_list;
static int android_dev_count;
static int android_bind_config(struct usb_configuration *c);
static void android_unbind_config(struct usb_configuration *c);
static struct android_dev *cdev_to_android_dev(struct usb_composite_dev *cdev);
static struct android_configuration *alloc_android_config
						(struct android_dev *dev);
static void free_android_config(struct android_dev *dev,
				struct android_configuration *conf);


#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
static struct delayed_work factory_work;
//static int is_factory_mode=0;
static int is_factory_called=0;
static struct android_dev *_android_dev;
#endif

#ifdef CONFIG_PANTECH_USB_BLOCKING_MDMSTATE
int is_mdm_usb_control_enabled = 0;
#endif

/* string IDs are assigned dynamically */
#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char manufacturer_string[256];
static char product_string[256];
static char serial_string[256];

/* String Table */
static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer_string,
	[STRING_PRODUCT_IDX].s = product_string,
	[STRING_SERIAL_IDX].s = serial_string,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
#ifdef CONFIG_PANTECH_ANDROID_USB
	.bDeviceClass         = USB_CLASS_COMM,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = 0x00,
#else
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
#endif
	.bNumConfigurations   = 1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
	.bcdOTG               = __constant_cpu_to_le16(0x0200),
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

enum android_device_state {
	USB_DISCONNECTED,
	USB_CONNECTED,
	USB_CONFIGURED,
};

static void android_pm_qos_update_latency(struct android_dev *dev, int vote)
{
	struct android_usb_platform_data *pdata = dev->pdata;
	u32 swfi_latency = 0;
	static int last_vote = -1;

	if (!pdata || vote == last_vote
		|| !pdata->swfi_latency)
		return;

	swfi_latency = pdata->swfi_latency + 1;
	if (vote)
		pm_qos_update_request(&dev->pm_qos_req_dma,
				swfi_latency);
	else
		pm_qos_update_request(&dev->pm_qos_req_dma,
				PM_QOS_DEFAULT_VALUE);
	last_vote = vote;
}

static void android_work(struct work_struct *data)
{
	struct android_dev *dev = container_of(data, struct android_dev, work);
	struct usb_composite_dev *cdev = dev->cdev;
	char *disconnected[2] = { "USB_STATE=DISCONNECTED", NULL };
	char *connected[2]    = { "USB_STATE=CONNECTED", NULL };
	char *configured[2]   = { "USB_STATE=CONFIGURED", NULL };
	char **uevent_envp = NULL;
	static enum android_device_state last_uevent, next_state;
	unsigned long flags;
	int pm_qos_vote = -1;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config) {
		uevent_envp = configured;
		next_state = USB_CONFIGURED;
	} else if (dev->connected != dev->sw_connected) {
		uevent_envp = dev->connected ? connected : disconnected;
		next_state = dev->connected ? USB_CONNECTED : USB_DISCONNECTED;
		if (dev->connected && strncmp(dev->pm_qos, "low", 3))
			pm_qos_vote = 1;
		else if (!dev->connected || !strncmp(dev->pm_qos, "low", 3))
			pm_qos_vote = 0;
	}
	dev->sw_connected = dev->connected;
	spin_unlock_irqrestore(&cdev->lock, flags);

	if (pm_qos_vote != -1)
		android_pm_qos_update_latency(dev, pm_qos_vote);

	if (uevent_envp) {
		/*
		 * Some userspace modules, e.g. MTP, work correctly only if
		 * CONFIGURED uevent is preceded by DISCONNECT uevent.
		 * Check if we missed sending out a DISCONNECT uevent. This can
		 * happen if host PC resets and configures device really quick.
		 */
		if (((uevent_envp == connected) &&
		      (last_uevent != USB_DISCONNECTED)) ||
		    ((uevent_envp == configured) &&
		      (last_uevent == USB_CONFIGURED))) {
			pr_info("%s: sent missed DISCONNECT event\n", __func__);
			kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE,
								disconnected);
			msleep(20);
		}
		/*
		 * Before sending out CONFIGURED uevent give function drivers
		 * a chance to wakeup userspace threads and notify disconnect
		 */
		if (uevent_envp == configured)
			msleep(50);

		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		last_uevent = next_state;
		pr_info("%s: sent uevent %s\n", __func__, uevent_envp[0]);
	} else {
		pr_info("%s: did not send uevent (%d %d %p)\n", __func__,
			 dev->connected, dev->sw_connected, cdev->config);
	}
}

static void android_enable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_configuration *conf;

	if (WARN_ON(!dev->disable_depth))
		return;

	if (--dev->disable_depth == 0) {

		list_for_each_entry(conf, &dev->configs, list_item)
			usb_add_config(cdev, &conf->usb_config,
						android_bind_config);

		usb_gadget_connect(cdev->gadget);
	}
}

static void android_disable(struct android_dev *dev)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_configuration *conf;

	if (dev->disable_depth++ == 0) {
		usb_gadget_disconnect(cdev->gadget);
		/* Cancel pending control requests */
		usb_ep_dequeue(cdev->gadget->ep0, cdev->req);

		list_for_each_entry(conf, &dev->configs, list_item)
			usb_remove_config(cdev, &conf->usb_config);
	}
}

/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

struct adb_data {
	bool opened;
	bool enabled;
};

static int
adb_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct adb_data), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return adb_setup();
}

static void adb_function_cleanup(struct android_usb_function *f)
{
	adb_cleanup();
	kfree(f->config);
}

static int
adb_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return adb_bind_config(c);
}

static void adb_android_function_enable(struct android_usb_function *f)
{
	struct android_dev *dev = f->android_dev;
	struct adb_data *data = f->config;

	data->enabled = true;

	/* Disable the gadget until adbd is ready */
	if (!data->opened)
		android_disable(dev);
}

static void adb_android_function_disable(struct android_usb_function *f)
{
	struct android_dev *dev = f->android_dev;
	struct adb_data *data = f->config;

	data->enabled = false;

	/* Balance the disable that was called in closed_callback */
	if (!data->opened)
		android_enable(dev);
}

static struct android_usb_function adb_function = {
	.name		= "adb",
	.enable		= adb_android_function_enable,
	.disable	= adb_android_function_disable,
	.init		= adb_function_init,
	.cleanup	= adb_function_cleanup,
	.bind_config	= adb_function_bind_config,
};

static void adb_ready_callback(void)
{
	struct android_dev *dev = adb_function.android_dev;
	struct adb_data *data = adb_function.config;

	data->opened = true;

	if (data->enabled && dev) {
		mutex_lock(&dev->mutex);
		android_enable(dev);
		mutex_unlock(&dev->mutex);
	}
}

static void adb_closed_callback(void)
{
	struct android_dev *dev = adb_function.android_dev;
	struct adb_data *data = adb_function.config;

	data->opened = false;

	if (data->enabled) {
		mutex_lock(&dev->mutex);
		android_disable(dev);
		mutex_unlock(&dev->mutex);
	}
}


/*-------------------------------------------------------------------------*/
/* Supported functions initialization */

/* RMNET_SMD */
static int rmnet_smd_function_bind_config(struct android_usb_function *f,
					  struct usb_configuration *c)
{
	return rmnet_smd_bind_config(c);
}

static struct android_usb_function rmnet_smd_function = {
	.name		= "rmnet_smd",
	.bind_config	= rmnet_smd_function_bind_config,
};

/* RMNET_SDIO */
static int rmnet_sdio_function_bind_config(struct android_usb_function *f,
					  struct usb_configuration *c)
{
	return rmnet_sdio_function_add(c);
}

static struct android_usb_function rmnet_sdio_function = {
	.name		= "rmnet_sdio",
	.bind_config	= rmnet_sdio_function_bind_config,
};

/* RMNET_SMD_SDIO */
static int rmnet_smd_sdio_function_init(struct android_usb_function *f,
				 struct usb_composite_dev *cdev)
{
	return rmnet_smd_sdio_init();
}

static void rmnet_smd_sdio_function_cleanup(struct android_usb_function *f)
{
	rmnet_smd_sdio_cleanup();
}

static int rmnet_smd_sdio_bind_config(struct android_usb_function *f,
					  struct usb_configuration *c)
{
	return rmnet_smd_sdio_function_add(c);
}

static struct device_attribute *rmnet_smd_sdio_attributes[] = {
					&dev_attr_transport, NULL };

static struct android_usb_function rmnet_smd_sdio_function = {
	.name		= "rmnet_smd_sdio",
	.init		= rmnet_smd_sdio_function_init,
	.cleanup	= rmnet_smd_sdio_function_cleanup,
	.bind_config	= rmnet_smd_sdio_bind_config,
	.attributes	= rmnet_smd_sdio_attributes,
};

/*rmnet transport string format(per port):"ctrl0,data0,ctrl1,data1..." */
#define MAX_XPORT_STR_LEN 50
static char rmnet_transports[MAX_XPORT_STR_LEN];

/*rmnet transport name string - "rmnet_hsic[,rmnet_hsusb]" */
static char rmnet_xport_names[MAX_XPORT_STR_LEN];

static void rmnet_function_cleanup(struct android_usb_function *f)
{
	frmnet_cleanup();
}

static int rmnet_function_bind_config(struct android_usb_function *f,
					 struct usb_configuration *c)
{
	int i;
	int err = 0;
	char *ctrl_name;
	char *data_name;
	char *tname = NULL;
	char buf[MAX_XPORT_STR_LEN], *b;
	char xport_name_buf[MAX_XPORT_STR_LEN], *tb;
	static int rmnet_initialized, ports;

	if (!rmnet_initialized) {
		rmnet_initialized = 1;
		strlcpy(buf, rmnet_transports, sizeof(buf));
		b = strim(buf);

		strlcpy(xport_name_buf, rmnet_xport_names,
				sizeof(xport_name_buf));
		tb = strim(xport_name_buf);

		while (b) {
			ctrl_name = strsep(&b, ",");
			data_name = strsep(&b, ",");
			if (ctrl_name && data_name) {
				if (tb)
					tname = strsep(&tb, ",");
				err = frmnet_init_port(ctrl_name, data_name,
						tname);
				if (err) {
					pr_err("rmnet: Cannot open ctrl port:"
						"'%s' data port:'%s'\n",
						ctrl_name, data_name);
					goto out;
				}
				ports++;
			}
		}

		err = rmnet_gport_setup();
		if (err) {
			pr_err("rmnet: Cannot setup transports");
			goto out;
		}
	}

	for (i = 0; i < ports; i++) {
		err = frmnet_bind_config(c, i);
		if (err) {
			pr_err("Could not bind rmnet%u config\n", i);
			break;
		}
	}
out:
	return err;
}

static ssize_t rmnet_transports_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", rmnet_transports);
}

static ssize_t rmnet_transports_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(rmnet_transports, buff, sizeof(rmnet_transports));

	return size;
}

static ssize_t rmnet_xport_names_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", rmnet_xport_names);
}

static ssize_t rmnet_xport_names_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(rmnet_xport_names, buff, sizeof(rmnet_xport_names));

	return size;
}

static struct device_attribute dev_attr_rmnet_transports =
					__ATTR(transports, S_IRUGO | S_IWUSR,
						rmnet_transports_show,
						rmnet_transports_store);

static struct device_attribute dev_attr_rmnet_xport_names =
				__ATTR(transport_names, S_IRUGO | S_IWUSR,
				rmnet_xport_names_show,
				rmnet_xport_names_store);

static struct device_attribute *rmnet_function_attributes[] = {
					&dev_attr_rmnet_transports,
					&dev_attr_rmnet_xport_names,
					NULL };

static struct android_usb_function rmnet_function = {
	.name		= "rmnet",
	.cleanup	= rmnet_function_cleanup,
	.bind_config	= rmnet_function_bind_config,
	.attributes	= rmnet_function_attributes,
};

struct ecm_function_config {
	u8      ethaddr[ETH_ALEN];
};

static int ecm_function_init(struct android_usb_function *f,
				struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct ecm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void ecm_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int ecm_qc_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int ret;
	struct ecm_function_config *ecm = f->config;

	if (!ecm) {
		pr_err("%s: ecm_pdata\n", __func__);
		return -EINVAL;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);

	ret = gether_qc_setup_name(c->cdev->gadget, ecm->ethaddr, "ecm");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	return ecm_qc_bind_config(c, ecm->ethaddr);
}

static void ecm_qc_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_qc_cleanup();
}

static ssize_t ecm_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;
	return snprintf(buf, PAGE_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);
}

static ssize_t ecm_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&ecm->ethaddr[0], (int *)&ecm->ethaddr[1],
		    (int *)&ecm->ethaddr[2], (int *)&ecm->ethaddr[3],
		    (int *)&ecm->ethaddr[4], (int *)&ecm->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ecm_ethaddr, S_IRUGO | S_IWUSR, ecm_ethaddr_show,
					       ecm_ethaddr_store);

static struct device_attribute *ecm_function_attributes[] = {
	&dev_attr_ecm_ethaddr,
	NULL
};

static struct android_usb_function ecm_qc_function = {
	.name		= "ecm_qc",
	.init		= ecm_function_init,
	.cleanup	= ecm_function_cleanup,
	.bind_config	= ecm_qc_function_bind_config,
	.unbind_config	= ecm_qc_function_unbind_config,
	.attributes	= ecm_function_attributes,
};

/* MBIM - used with BAM */
#define MAX_MBIM_INSTANCES 1

static int mbim_function_init(struct android_usb_function *f,
					 struct usb_composite_dev *cdev)
{
	return mbim_init(MAX_MBIM_INSTANCES);
}

static void mbim_function_cleanup(struct android_usb_function *f)
{
	fmbim_cleanup();
}

static int mbim_function_bind_config(struct android_usb_function *f,
					  struct usb_configuration *c)
{
	return mbim_bind_config(c, 0);
}

static struct android_usb_function mbim_function = {
	.name		= "usb_mbim",
	.cleanup	= mbim_function_cleanup,
	.bind_config	= mbim_function_bind_config,
	.init		= mbim_function_init,
};


/* DIAG */
static char diag_clients[32];	    /*enabled DIAG clients- "diag[,diag_mdm]" */
static ssize_t clients_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(diag_clients, buff, sizeof(diag_clients));

	return size;
}

static DEVICE_ATTR(clients, S_IWUSR, NULL, clients_store);
static struct device_attribute *diag_function_attributes[] =
					 { &dev_attr_clients, NULL };

static int diag_function_init(struct android_usb_function *f,
				 struct usb_composite_dev *cdev)
{
	return diag_setup();
}

static void diag_function_cleanup(struct android_usb_function *f)
{
	diag_cleanup();
}

static int diag_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	char *name;
	char buf[32], *b;
	int once = 0, err = -1;
	int (*notify)(uint32_t, const char *);
	struct android_dev *dev = cdev_to_android_dev(c->cdev);

	strlcpy(buf, diag_clients, sizeof(buf));
	b = strim(buf);

	while (b) {
		notify = NULL;
		name = strsep(&b, ",");
		/* Allow only first diag channel to update pid and serial no */
		if (dev->pdata && !once++)
			notify = dev->pdata->update_pid_and_serial_num;

		if (name) {
			err = diag_function_add(c, name, notify);
			if (err)
				pr_err("diag: Cannot open channel '%s'", name);
		}
	}

	return err;
}

static struct android_usb_function diag_function = {
	.name		= "diag",
	.init		= diag_function_init,
	.cleanup	= diag_function_cleanup,
	.bind_config	= diag_function_bind_config,
	.attributes	= diag_function_attributes,
};

/* DEBUG */
static int qdss_function_init(struct android_usb_function *f,
	struct usb_composite_dev *cdev)
{
	return qdss_setup();
}

static void qdss_function_cleanup(struct android_usb_function *f)
{
	qdss_cleanup();
}

static int qdss_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int  err = -1;

	err = qdss_bind_config(c, "qdss");
	if (err)
		pr_err("qdss: Cannot open channel qdss");

	return err;
}

static struct android_usb_function qdss_function = {
	.name		= "qdss",
	.init		= qdss_function_init,
	.cleanup	= qdss_function_cleanup,
	.bind_config	= qdss_function_bind_config,
};

/* SERIAL */
static char serial_transports[32];	/*enabled FSERIAL ports - "tty[,sdio]"*/
static ssize_t serial_transports_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(serial_transports, buff, sizeof(serial_transports));

	return size;
}

/*enabled FSERIAL transport names - "serial_hsic[,serial_hsusb]"*/
static char serial_xport_names[32];
static ssize_t serial_xport_names_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(serial_xport_names, buff, sizeof(serial_xport_names));

	return size;
}

static ssize_t serial_xport_names_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", serial_xport_names);
}

static DEVICE_ATTR(transports, S_IWUSR, NULL, serial_transports_store);
static struct device_attribute dev_attr_serial_xport_names =
				__ATTR(transport_names, S_IRUGO | S_IWUSR,
				serial_xport_names_show,
				serial_xport_names_store);

static struct device_attribute *serial_function_attributes[] = {
					&dev_attr_transports,
					&dev_attr_serial_xport_names,
					NULL };

static void serial_function_cleanup(struct android_usb_function *f)
{
	gserial_cleanup();
}

static int serial_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	char *name, *xport_name = NULL;
	char buf[32], *b, xport_name_buf[32], *tb;
	int err = -1, i;
	static int serial_initialized = 0, ports = 0;

	if (serial_initialized)
		goto bind_config;

	serial_initialized = 1;
	strlcpy(buf, serial_transports, sizeof(buf));
	b = strim(buf);

	strlcpy(xport_name_buf, serial_xport_names, sizeof(xport_name_buf));
	tb = strim(xport_name_buf);

	while (b) {
		name = strsep(&b, ",");

		if (name) {
			if (tb)
				xport_name = strsep(&tb, ",");
			err = gserial_init_port(ports, name, xport_name);
			if (err) {
				pr_err("serial: Cannot open port '%s'", name);
				goto out;
			}
			ports++;
		}
	}
	err = gport_setup(c);
	if (err) {
		pr_err("serial: Cannot setup transports");

#ifdef FEATURE_PANTECH_USB_FIELDTEST_WORKAROUND
		if (device_desc.idProduct == 0x9096){
			printk("%s, QC field test mode. PID 9096\n", __func__);
			goto bind_config;
		}
#endif

		goto out;
	}

bind_config:
	for (i = 0; i < ports; i++) {
		err = gser_bind_config(c, i);
		if (err) {
			pr_err("serial: bind_config failed for port %d", i);
			goto out;
		}
	}

out:
	return err;
}

static struct android_usb_function serial_function = {
	.name		= "serial",
	.cleanup	= serial_function_cleanup,
	.bind_config	= serial_function_bind_config,
	.attributes	= serial_function_attributes,
};

/* ACM */
static char acm_transports[32];	/*enabled ACM ports - "tty[,sdio]"*/
static ssize_t acm_transports_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(acm_transports, buff, sizeof(acm_transports));

	return size;
}

static DEVICE_ATTR(acm_transports, S_IWUSR, NULL, acm_transports_store);

/*enabled ACM transport names - "serial_hsic[,serial_hsusb]"*/
static char acm_xport_names[32];
static ssize_t acm_xport_names_store(
		struct device *device, struct device_attribute *attr,
		const char *buff, size_t size)
{
	strlcpy(acm_xport_names, buff, sizeof(acm_xport_names));

	return size;
}

static DEVICE_ATTR(acm_transport_names, S_IWUSR, NULL, acm_xport_names_store);

static struct device_attribute *acm_function_attributes[] = {
		&dev_attr_acm_transports,
		&dev_attr_acm_transport_names,
		NULL };

static void acm_function_cleanup(struct android_usb_function *f)
{
	gserial_cleanup();
}

static int acm_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	char *name, *xport_name = NULL;
	char buf[32], *b, xport_name_buf[32], *tb;
	int err = -1, i;
	static int acm_initialized, ports;

	if (acm_initialized)
		goto bind_config;

	acm_initialized = 1;
	strlcpy(buf, acm_transports, sizeof(buf));
	b = strim(buf);

	strlcpy(xport_name_buf, acm_xport_names, sizeof(xport_name_buf));
	tb = strim(xport_name_buf);

	while (b) {
		name = strsep(&b, ",");

		if (name) {
			if (tb)
				xport_name = strsep(&tb, ",");
			err = acm_init_port(ports, name, xport_name);
			if (err) {
				pr_err("acm: Cannot open port '%s'", name);
				goto out;
			}
			ports++;
		}
	}
	err = acm_port_setup(c);
	if (err) {
		pr_err("acm: Cannot setup transports");
		goto out;
	}

bind_config:
	for (i = 0; i < ports; i++) {
		err = acm_bind_config(c, i);
		if (err) {
			pr_err("acm: bind_config failed for port %d", i);
			goto out;
		}
	}

out:
	return err;
}
static struct android_usb_function acm_function = {
	.name		= "acm",
	.cleanup	= acm_function_cleanup,
	.bind_config	= acm_function_bind_config,
	.attributes	= acm_function_attributes,
};

#ifdef CONFIG_PANTECH_ANDROID_OBEX
/* PANTECH OBEX */
static int pantech_obex_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	pr_debug("%s pantech_obex init function nothing work!!!\n", __func__);
	pantech_obex_setup();
	return 0;
}

static void pantech_obex_function_cleanup(struct android_usb_function *f)
{
	pr_debug("%s pantech_obex cleanup function nothing work!!!\n", __func__);
	pantech_obex_cleanup();
}

static int pantech_obex_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	return pantech_obex_bind_config(c);
}

static struct android_usb_function pantech_obex_function = {
	.name		= "obex",
	.init		= pantech_obex_function_init,
	.cleanup	= pantech_obex_function_cleanup,
	.bind_config	= pantech_obex_function_bind_config,
};
#endif

/* CCID */
static int ccid_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return ccid_setup();
}

static void ccid_function_cleanup(struct android_usb_function *f)
{
	ccid_cleanup();
}

static int ccid_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return ccid_bind_config(c);
}

static struct android_usb_function ccid_function = {
	.name		= "ccid",
	.init		= ccid_function_init,
	.cleanup	= ccid_function_cleanup,
	.bind_config	= ccid_function_bind_config,
};

static int mtp_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	return mtp_setup();
}

static void mtp_function_cleanup(struct android_usb_function *f)
{
	mtp_cleanup();
}

static int mtp_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	return mtp_bind_config(c, false);
}

static int ptp_function_init(struct android_usb_function *f, struct usb_composite_dev *cdev)
{
	/* nothing to do - initialization is handled by mtp_function_init */
	return 0;
}

static void ptp_function_cleanup(struct android_usb_function *f)
{
	/* nothing to do - cleanup is handled by mtp_function_cleanup */
}

static int ptp_function_bind_config(struct android_usb_function *f, struct usb_configuration *c)
{
	return mtp_bind_config(c, true);
}

static int mtp_function_ctrlrequest(struct android_usb_function *f,
					struct usb_composite_dev *cdev,
					const struct usb_ctrlrequest *c)
{
	return mtp_ctrlrequest(cdev, c);
}

static struct android_usb_function mtp_function = {
	.name		= "mtp",
	.init		= mtp_function_init,
	.cleanup	= mtp_function_cleanup,
	.bind_config	= mtp_function_bind_config,
	.ctrlrequest	= mtp_function_ctrlrequest,
};

/* PTP function is same as MTP with slightly different interface descriptor */
static struct android_usb_function ptp_function = {
	.name		= "ptp",
	.init		= ptp_function_init,
	.cleanup	= ptp_function_cleanup,
	.bind_config	= ptp_function_bind_config,
};

#ifdef CONFIG_USB_ANDROID_CDC_ECM
struct ecm_function_config {
	u8 ethaddr[ETH_ALEN];
	u32 vendorID;
	char manufacturer[256];
};

static int ecm_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct ecm_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void ecm_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int ecm_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int ret;
	struct ecm_function_config *ecm = f->config;

	if (!ecm) {
		pr_err("%s: ecm_function_config\n", __func__);
		return -1;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);

	ret = gether_setup_name(c->cdev->gadget, ecm->ethaddr, "usb");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	return ecm_bind_config(c, ecm->ethaddr);
}

static void ecm_function_unbind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	gether_cleanup();
}

static ssize_t ecm_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;

	return snprintf(buf, PAGE_SIZE, "%s\n", config->manufacturer);
}

static ssize_t ecm_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;

	if (sscanf(buf, "%255s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufacturer, S_IRUGO | S_IWUSR, ecm_manufacturer_show,
						    ecm_manufacturer_store);

static ssize_t ecm_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;
	return sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		ecm->ethaddr[0], ecm->ethaddr[1], ecm->ethaddr[2],
		ecm->ethaddr[3], ecm->ethaddr[4], ecm->ethaddr[5]);
}

static ssize_t ecm_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *ecm = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
			(int *)&ecm->ethaddr[0], (int *)&ecm->ethaddr[1],
			(int *)&ecm->ethaddr[2], (int *)&ecm->ethaddr[3],
			(int *)&ecm->ethaddr[4], (int *)&ecm->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, ecm_ethaddr_show,
		ecm_ethaddr_store);

static ssize_t ecm_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;

	return snprintf(buf, PAGE_SIZE, "%04x\n", config->vendorID);
}

static ssize_t ecm_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct ecm_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendorID, S_IRUGO | S_IWUSR, ecm_vendorID_show,
						ecm_vendorID_store);

static struct device_attribute *ecm_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	NULL
};

static struct android_usb_function ecm_function = {
	.name = "ecm",
	.init = ecm_function_init,
	.cleanup = ecm_function_cleanup,
	.bind_config = ecm_function_bind_config,
	.unbind_config = ecm_function_unbind_config,
	.attributes = ecm_function_attributes,
};

#else

struct rndis_function_config {
	u8      ethaddr[ETH_ALEN];
	u32     vendorID;
	u8      max_pkt_per_xfer;
	char	manufacturer[256];
	/* "Wireless" RNDIS; auto-detected by Windows */
	bool	wceis;
};

static int
rndis_function_init(struct android_usb_function *f,
		struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;
	return 0;
}

static void rndis_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int rndis_qc_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	f->config = kzalloc(sizeof(struct rndis_function_config), GFP_KERNEL);
	if (!f->config)
		return -ENOMEM;

	return rndis_qc_init();
}

static void rndis_qc_function_cleanup(struct android_usb_function *f)
{
	rndis_qc_cleanup();
	kfree(f->config);
}

static int
rndis_function_bind_config(struct android_usb_function *f,
		struct usb_configuration *c)
{
	int ret;
	struct rndis_function_config *rndis = f->config;

	if (!rndis) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -1;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);

	ret = gether_setup_name(c->cdev->gadget, rndis->ethaddr, "usb");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	if (rndis->wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_control_intf.bInterfaceSubClass =	 0x01;
		rndis_control_intf.bInterfaceProtocol =	 0x03;
	}

	return rndis_bind_config_vendor(c, rndis->ethaddr, rndis->vendorID,
					   rndis->manufacturer);
}

static int rndis_qc_function_bind_config(struct android_usb_function *f,
					struct usb_configuration *c)
{
	int ret;
	struct rndis_function_config *rndis = f->config;

	if (!rndis) {
		pr_err("%s: rndis_pdata\n", __func__);
		return -EINVAL;
	}

	pr_info("%s MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);

	ret = gether_qc_setup_name(c->cdev->gadget, rndis->ethaddr, "rndis");
	if (ret) {
		pr_err("%s: gether_setup failed\n", __func__);
		return ret;
	}

	if (rndis->wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_qc_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_qc_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_qc_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_qc_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_qc_control_intf.bInterfaceSubClass =	 0x01;
		rndis_qc_control_intf.bInterfaceProtocol =	 0x03;
	}

	return rndis_qc_bind_config_vendor(c, rndis->ethaddr, rndis->vendorID,
				    rndis->manufacturer,
					rndis->max_pkt_per_xfer);
}

static void rndis_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_cleanup();
}

static void rndis_qc_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	gether_qc_cleanup();
}

static ssize_t rndis_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	return snprintf(buf, PAGE_SIZE, "%s\n", config->manufacturer);
}

static ssize_t rndis_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	if (size >= sizeof(config->manufacturer))
		return -EINVAL;

	if (sscanf(buf, "%255s", config->manufacturer) == 1)
		return size;
	return -1;
}

static DEVICE_ATTR(manufacturer, S_IRUGO | S_IWUSR, rndis_manufacturer_show,
						    rndis_manufacturer_store);

static ssize_t rndis_wceis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	return snprintf(buf, PAGE_SIZE, "%d\n", config->wceis);
}

static ssize_t rndis_wceis_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%d", &value) == 1) {
		config->wceis = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(wceis, S_IRUGO | S_IWUSR, rndis_wceis_show,
					     rndis_wceis_store);

static ssize_t rndis_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;

	return snprintf(buf, PAGE_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		rndis->ethaddr[0], rndis->ethaddr[1], rndis->ethaddr[2],
		rndis->ethaddr[3], rndis->ethaddr[4], rndis->ethaddr[5]);
}

static ssize_t rndis_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *rndis = f->config;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    (int *)&rndis->ethaddr[0], (int *)&rndis->ethaddr[1],
		    (int *)&rndis->ethaddr[2], (int *)&rndis->ethaddr[3],
		    (int *)&rndis->ethaddr[4], (int *)&rndis->ethaddr[5]) == 6)
		return size;
	return -EINVAL;
}

static DEVICE_ATTR(ethaddr, S_IRUGO | S_IWUSR, rndis_ethaddr_show,
					       rndis_ethaddr_store);

static ssize_t rndis_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;

	return snprintf(buf, PAGE_SIZE, "%04x\n", config->vendorID);
}

static ssize_t rndis_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%04x", &value) == 1) {
		config->vendorID = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(vendorID, S_IRUGO | S_IWUSR, rndis_vendorID_show,
						rndis_vendorID_store);

static ssize_t rndis_max_pkt_per_xfer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->max_pkt_per_xfer);
}

static ssize_t rndis_max_pkt_per_xfer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct rndis_function_config *config = f->config;
	int value;

	if (sscanf(buf, "%d", &value) == 1) {
		config->max_pkt_per_xfer = value;
		return size;
	}
	return -EINVAL;
}

static DEVICE_ATTR(max_pkt_per_xfer, S_IRUGO | S_IWUSR,
				   rndis_max_pkt_per_xfer_show,
				   rndis_max_pkt_per_xfer_store);

static struct device_attribute *rndis_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_wceis,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	&dev_attr_max_pkt_per_xfer,
	NULL
};

static struct android_usb_function rndis_function = {
	.name		= "rndis",
	.init		= rndis_function_init,
	.cleanup	= rndis_function_cleanup,
	.bind_config	= rndis_function_bind_config,
	.unbind_config	= rndis_function_unbind_config,
	.attributes	= rndis_function_attributes,
};
#endif /* CONFIG_USB_ANDROID_CDC_ECM */

static struct android_usb_function rndis_qc_function = {
	.name		= "rndis_qc",
	.init		= rndis_qc_function_init,
	.cleanup	= rndis_qc_function_cleanup,
	.bind_config	= rndis_qc_function_bind_config,
	.unbind_config	= rndis_qc_function_unbind_config,
	.attributes	= rndis_function_attributes,
};

struct mass_storage_function_config {
	struct fsg_config fsg;
	struct fsg_common *common;
};

static int mass_storage_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	struct android_dev *dev = cdev_to_android_dev(cdev);
	struct mass_storage_function_config *config;
	struct fsg_common *common;
	int err;
	int i;
	const char *name[2];

	config = kzalloc(sizeof(struct mass_storage_function_config),
								GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	config->fsg.nluns = 1;
	name[0] = "lun";
	if (dev->pdata && dev->pdata->cdrom) {
		config->fsg.nluns = 2;
		config->fsg.luns[1].cdrom = 1;
		config->fsg.luns[1].ro = 1;
		config->fsg.luns[1].removable = 1;
		name[1] = "lun0";
	}

	config->fsg.luns[0].removable = 1;
#ifdef CONFIG_PANTECH_ANDROID_USB
	config->fsg.vendor_name = "Pantech";
	config->fsg.product_name = "MStorage";
#endif
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
	config->fsg.luns[config->fsg.nluns].removable = 1;
	config->fsg.luns[config->fsg.nluns].cdrom = 1;
	config->fsg.luns[config->fsg.nluns].ro = 1;
    name[config->fsg.nluns] = "lun0";
	config->fsg.nluns++;
#endif

	common = fsg_common_init(NULL, cdev, &config->fsg);
	if (IS_ERR(common)) {
		kfree(config);
		return PTR_ERR(common);
	}

	for (i = 0; i < config->fsg.nluns; i++) {
		err = sysfs_create_link(&f->dev->kobj,
					&common->luns[i].dev.kobj,
					name[i]);
		if (err)
			goto error;
	}

	config->common = common;
	f->config = config;
	return 0;
error:
	for (; i > 0 ; i--)
		sysfs_remove_link(&f->dev->kobj, name[i-1]);

	fsg_common_release(&common->ref);
	kfree(config);
	return err;
}

static void mass_storage_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
	f->config = NULL;
}

static int mass_storage_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct mass_storage_function_config *config = f->config;
	return fsg_bind_config(c->cdev, c, config->common);
}

static ssize_t mass_storage_inquiry_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%s\n", config->common->inquiry_string);
}

static ssize_t mass_storage_inquiry_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	if (size >= sizeof(config->common->inquiry_string))
		return -EINVAL;
	if (sscanf(buf, "%28s", config->common->inquiry_string) != 1)
		return -EINVAL;
	return size;
}

static DEVICE_ATTR(inquiry_string, S_IRUGO | S_IWUSR,
					mass_storage_inquiry_show,
					mass_storage_inquiry_store);

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
static ssize_t mass_storage_cdrom_lun_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct mass_storage_function_config *config = f->config;
	return snprintf(buf, PAGE_SIZE, "%d\n", config->fsg.nluns - 1);
}

static DEVICE_ATTR(cdrom_lun, S_IRUGO, mass_storage_cdrom_lun_show, NULL);
#endif

static struct device_attribute *mass_storage_function_attributes[] = {
	&dev_attr_inquiry_string,
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
	&dev_attr_cdrom_lun,
#endif
	NULL
};

static struct android_usb_function mass_storage_function = {
	.name		= "mass_storage",
	.init		= mass_storage_function_init,
	.cleanup	= mass_storage_function_cleanup,
	.bind_config	= mass_storage_function_bind_config,
	.attributes	= mass_storage_function_attributes,
};


static int accessory_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return acc_setup();
}

static void accessory_function_cleanup(struct android_usb_function *f)
{
	acc_cleanup();
}

static int accessory_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return acc_bind_config(c);
}

static int accessory_function_ctrlrequest(struct android_usb_function *f,
						struct usb_composite_dev *cdev,
						const struct usb_ctrlrequest *c)
{
	return acc_ctrlrequest(cdev, c);
}

static struct android_usb_function accessory_function = {
	.name		= "accessory",
	.init		= accessory_function_init,
	.cleanup	= accessory_function_cleanup,
	.bind_config	= accessory_function_bind_config,
	.ctrlrequest	= accessory_function_ctrlrequest,
};

static int audio_source_function_init(struct android_usb_function *f,
			struct usb_composite_dev *cdev)
{
	struct audio_source_config *config;

	config = kzalloc(sizeof(struct audio_source_config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;
	config->card = -1;
	config->device = -1;
	f->config = config;
	return 0;
}

static void audio_source_function_cleanup(struct android_usb_function *f)
{
	kfree(f->config);
}

static int audio_source_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	return audio_source_bind_config(c, config);
}

static void audio_source_function_unbind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	struct audio_source_config *config = f->config;

	config->card = -1;
	config->device = -1;
}

static ssize_t audio_source_pcm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	struct audio_source_config *config = f->config;

	/* print PCM card and device numbers */
	return sprintf(buf, "%d %d\n", config->card, config->device);
}

static DEVICE_ATTR(pcm, S_IRUGO | S_IWUSR, audio_source_pcm_show, NULL);

static struct device_attribute *audio_source_function_attributes[] = {
	&dev_attr_pcm,
	NULL
};

static struct android_usb_function audio_source_function = {
	.name		= "audio_source",
	.init		= audio_source_function_init,
	.cleanup	= audio_source_function_cleanup,
	.bind_config	= audio_source_function_bind_config,
	.unbind_config	= audio_source_function_unbind_config,
	.attributes	= audio_source_function_attributes,
};

static int android_uasp_connect_cb(bool connect)
{
	/*
	 * TODO
	 * We may have to disable gadget till UASP configfs nodes
	 * are configured which includes mapping LUN with the
	 * backing file. It is a fundamental difference between
	 * f_mass_storage and f_tcp. That means UASP can not be
	 * in default composition.
	 *
	 * For now, assume that UASP configfs nodes are configured
	 * before enabling android gadget. Or cable should be
	 * reconnected after mapping the LUN.
	 *
	 * Also consider making UASP to respond to Host requests when
	 * Lun is not mapped.
	 */
	pr_debug("UASP %s\n", connect ? "connect" : "disconnect");

	return 0;
}

static int uasp_function_init(struct android_usb_function *f,
					struct usb_composite_dev *cdev)
{
	return f_tcm_init(&android_uasp_connect_cb);
}

static void uasp_function_cleanup(struct android_usb_function *f)
{
	f_tcm_exit();
}

static int uasp_function_bind_config(struct android_usb_function *f,
						struct usb_configuration *c)
{
	return tcm_bind_config(c);
}

static struct android_usb_function uasp_function = {
	.name		= "uasp",
	.init		= uasp_function_init,
	.cleanup	= uasp_function_cleanup,
	.bind_config	= uasp_function_bind_config,
};

static struct android_usb_function *supported_functions[] = {
	&mbim_function,
	&ecm_qc_function,
	&rmnet_smd_function,
	&rmnet_sdio_function,
	&rmnet_smd_sdio_function,
	&rmnet_function,
	&diag_function,
	&qdss_function,
	&serial_function,
	&adb_function,
	&ccid_function,
	&acm_function,
	&mtp_function,
	&ptp_function,
#ifdef CONFIG_USB_ANDROID_CDC_ECM
	&ecm_function,
#else
	&rndis_function,
#endif
	&rndis_qc_function,
	&mass_storage_function,
	&accessory_function,
	&audio_source_function,
	&uasp_function,
#ifdef CONFIG_PANTECH_ANDROID_OBEX
	&pantech_obex_function,
#endif
	NULL
};

static void android_cleanup_functions(struct android_usb_function **functions)
{
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;

	while (*functions) {
		f = *functions++;

		if (f->dev) {
			device_destroy(android_class, f->dev->devt);
			kfree(f->dev_name);
		} else
			continue;

		if (f->cleanup)
			f->cleanup(f);

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++))
				device_remove_file(f->dev, attr);
		}
	}
}

static int android_init_functions(struct android_usb_function **functions,
				  struct usb_composite_dev *cdev)
{
	struct android_dev *dev = cdev_to_android_dev(cdev);
	struct android_usb_function *f;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;
	int index = 1; /* index 0 is for android0 device */

	for (; (f = *functions++); index++) {
		f->dev_name = kasprintf(GFP_KERNEL, "f_%s", f->name);
		f->android_dev = NULL;
		if (!f->dev_name) {
			err = -ENOMEM;
			goto err_out;
		}
		f->dev = device_create(android_class, dev->dev,
				MKDEV(0, index), f, f->dev_name);
		if (IS_ERR(f->dev)) {
			pr_err("%s: Failed to create dev %s", __func__,
							f->dev_name);
			err = PTR_ERR(f->dev);
			f->dev = NULL;
			goto err_create;
		}

		if (f->init) {
			err = f->init(f, cdev);
			if (err) {
				pr_err("%s: Failed to init %s", __func__,
								f->name);
				goto err_init;
			}
		}

		attrs = f->attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(f->dev, attr);
		}
		if (err) {
			pr_err("%s: Failed to create function %s attributes",
					__func__, f->name);
			goto err_attrs;
		}
	}
	return 0;

err_attrs:
	for (attr = *(attrs -= 2); attrs != f->attributes; attr = *(attrs--))
		device_remove_file(f->dev, attr);
	if (f->cleanup)
		f->cleanup(f);
err_init:
	device_destroy(android_class, f->dev->devt);
err_create:
	f->dev = NULL;
	kfree(f->dev_name);
err_out:
	android_cleanup_functions(dev->functions);
	return err;
}

static int
android_bind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	struct android_configuration *conf =
		container_of(c, struct android_configuration, usb_config);
	int ret;

	list_for_each_entry(f, &conf->enabled_functions, enabled_list) {
		ret = f->bind_config(f, c);
		if (ret) {
			pr_err("%s: %s failed", __func__, f->name);
			return ret;
		}
	}
	return 0;
}

static void
android_unbind_enabled_functions(struct android_dev *dev,
			       struct usb_configuration *c)
{
	struct android_usb_function *f;
	struct android_configuration *conf =
		container_of(c, struct android_configuration, usb_config);

	list_for_each_entry(f, &conf->enabled_functions, enabled_list) {
		if (f->unbind_config)
			f->unbind_config(f, c);
	}
}

static int android_enable_function(struct android_dev *dev,
				   struct android_configuration *conf,
				   char *name)
{
	struct android_usb_function **functions = dev->functions;
	struct android_usb_function *f;
	while ((f = *functions++)) {
		if (!strcmp(name, f->name)) {
			if (f->android_dev)
				pr_err("%s already enabled in other " \
					"configuration or device\n",
					f->name);
			else {
				list_add_tail(&f->enabled_list,
					      &conf->enabled_functions);
				f->android_dev = dev;
				return 0;
			}
		}
	}
	return -EINVAL;
}


#ifdef CONFIG_PANTECH_ANDROID_MTP
static int function_is_enabled(struct android_dev *dev, char *name){
	struct android_usb_function *f;
	struct android_configuration *conf;

	list_for_each_entry(conf, &dev->configs, list_item) {
		list_for_each_entry(f, &conf->enabled_functions, enabled_list){
			if(!strcmp(f->name, name)){
				return 1;
			}
		}
	}
	return 0;
}
#endif

/*-------------------------------------------------------------------------*/
/* /sys/class/android_usb/android%d/ interface */

static ssize_t remote_wakeup_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_configuration *conf;

	/*
	 * Show the wakeup attribute of the first configuration,
	 * since all configurations have the same wakeup attribute
	 */
	if (dev->configs_num == 0)
		return 0;
	conf = list_entry(dev->configs.next,
			  struct android_configuration,
			  list_item);

	return snprintf(buf, PAGE_SIZE, "%d\n",
			!!(conf->usb_config.bmAttributes &
				USB_CONFIG_ATT_WAKEUP));
}

static ssize_t remote_wakeup_store(struct device *pdev,
		struct device_attribute *attr, const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_configuration *conf;
	int enable = 0;

	sscanf(buff, "%d", &enable);

	pr_debug("android_usb: %s remote wakeup\n",
			enable ? "enabling" : "disabling");

	list_for_each_entry(conf, &dev->configs, list_item)
		if (enable)
			conf->usb_config.bmAttributes |=
					USB_CONFIG_ATT_WAKEUP;
		else
			conf->usb_config.bmAttributes &=
					~USB_CONFIG_ATT_WAKEUP;

	return size;
}

static ssize_t
functions_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_configuration *conf;
	struct android_usb_function *f;
	char *buff = buf;

	mutex_lock(&dev->mutex);

	list_for_each_entry(conf, &dev->configs, list_item) {
		if (buff != buf)
			*(buff-1) = ':';
		list_for_each_entry(f, &conf->enabled_functions, enabled_list)
			buff += snprintf(buff, PAGE_SIZE, "%s,", f->name);
	}

	mutex_unlock(&dev->mutex);

	if (buff != buf)
		*(buff-1) = '\n';
	return buff - buf;
}

#ifdef CONFIG_PANTECH_ANDROID_OTG
extern void set_otg_switch_ctl(int gpio, int value);
extern void read_line(void);
extern void read_adc(void);
#endif /* CONFIG_PANTECH_ANDROID_OTG */

static ssize_t
functions_store(struct device *pdev, struct device_attribute *attr,
			       const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct android_usb_function *f;
	struct list_head *curr_conf = &dev->configs;
	struct android_configuration *conf;
	char *conf_str;
	char *name;
	char buf[256], *b;
	int err;

	mutex_lock(&dev->mutex);

	if (dev->enabled) {
		mutex_unlock(&dev->mutex);
		return -EBUSY;
	}

	/* Clear previous enabled list */
	list_for_each_entry(conf, &dev->configs, list_item) {
		list_for_each_entry(f, &conf->enabled_functions, enabled_list)
			f->android_dev = NULL;
		INIT_LIST_HEAD(&conf->enabled_functions);
	}

	strlcpy(buf, buff, sizeof(buf));

#ifdef CONFIG_PANTECH_ANDROID_OTG
	if (!strncmp(buf, "360", 3)) {
		dev_dbg(pdev, "^^ 36, 0\n");
		set_otg_switch_ctl(36,0);
	}else if (!strncmp(buf, "361", 3)) {
		dev_dbg(pdev, "^^ 36, 1\n");
		set_otg_switch_ctl(36,1);
	}

	else if (!strncmp(buf, "440", 3)) {
		dev_dbg(pdev, "^^ 44, 0\n");
		set_otg_switch_ctl(44,0);
	}else if (!strncmp(buf, "441", 3)) {
		dev_dbg(pdev ,"^^ 44, 1\n");
		set_otg_switch_ctl(44,1);
	}

	else if (!strncmp(buf, "lin", 3)) {
		read_line();
	}

	else if (!strncmp(buf, "adc", 3)) {
		read_adc();
	}

	else if (!strncmp(buf, "pu0", 3)) {
		pm8xxx_usb_id_pullup(0);
	}else if (!strncmp(buf, "pu1", 3)) {
		pm8xxx_usb_id_pullup(1);
	}
#endif /* CONFIG_PANTECH_ANDROID_OTG */

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
    // Initialize the variables
	pantech_set_cdrom_enabled(0, 0);
    pantech_ext_config_desc_list[0].desc.function[0].bFirstInterfaceNumber = 0;
    mtp_ext_config_desc.function.bFirstInterfaceNumber = 0;
#endif

	b = strim(buf);

	while (b) {
		conf_str = strsep(&b, ":");
		if (conf_str) {
			/* If the next not equal to the head, take it */
			if (curr_conf->next != &dev->configs)
				conf = list_entry(curr_conf->next,
						  struct android_configuration,
						  list_item);
			else
				conf = alloc_android_config(dev);

			curr_conf = curr_conf->next;
		}

		while (conf_str) {
			name = strsep(&conf_str, ",");
			if (name) {

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
				if (!strcmp(name, "cdrom")) {
					err = android_enable_function(dev, conf, "mass_storage");
					pantech_set_cdrom_enabled(1, 1); // cdrom only
				} else {
					err = android_enable_function(dev, conf, name);
				}
#else
				err = android_enable_function(dev, conf, name);
#endif
				if (err)
					pr_err("android_usb: Cannot enable %s",
						name);
			}
		}
	}

	/* Free uneeded configurations if exists */
	while (curr_conf->next != &dev->configs) {
		conf = list_entry(curr_conf->next,
				  struct android_configuration, list_item);
		free_android_config(dev, conf);
	}

	mutex_unlock(&dev->mutex);

	return size;
}

static ssize_t enable_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", dev->enabled);
}

#if defined(CONFIG_PANTECH_SMB347_CHARGER)
int android_get_udc_state(void)
{
	struct android_dev *dev = _android_dev;
	struct usb_composite_dev *cdev;
	int ret;

	if (_android_dev == NULL || _android_dev->cdev == NULL) {
		ret = 3;
	} else {
		cdev = dev->cdev;
		if (cdev->config) {
			ret = 1;
		} else {
			ret = 0;
		}
	}

	return ret;
}
#endif

#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
static ssize_t factory_mode_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	//struct android_dev *dev = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d\n", is_factory_mode);
}


static ssize_t factory_mode_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{

	sscanf(buff, "%d", &is_factory_mode);
	//printk("^^ factory %d\n",is_factory_mode);
	return size;
}

int set_factory_mode(void){
	struct android_dev *dev = _android_dev;
	struct usb_composite_dev *cdev = dev->cdev;
	struct list_head *curr_conf = &dev->configs;
	struct android_configuration *conf;
	struct android_usb_function *f;
	char *conf_str;
	char *name;
	char buf[256], *b;
	int err = 0;

	static char *product_string_factory = "Android(FactoryMode)";
	int is_adb_enabled = 0;

	printk("set_factory_mode\n");
	if(dev == NULL){
		printk("dev is NULL\n");
		return 0;
	}
	is_adb_enabled = function_is_enabled(dev , "adb"); /* This function is called before usb_remove_config */

	if(is_adb_enabled){
		strlcpy(buf, "diag,adb,serial", sizeof(buf));
	}else{
		strlcpy(buf, "diag,serial", sizeof(buf));
	}
#if 0
	usb_gadget_disconnect(cdev->gadget);

	/* Cancel pending control requests */
	usb_ep_dequeue(cdev->gadget->ep0, cdev->req);

	list_for_each_entry(conf, &dev->configs, list_item)
			usb_remove_config(cdev, &conf->usb_config);

	dev->enabled = false;
#else
	android_disable(dev);

	list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f, &conf->enabled_functions,
						enabled_list) {
				if (f->disable)
					f->disable(f);
			}

	dev->enabled = false;
#endif

	strings_dev[STRING_PRODUCT_IDX].s = product_string_factory;

	strlcpy(diag_clients, "diag,diag_mdm", sizeof(diag_clients));
	strlcpy(serial_transports, "hsic", sizeof(serial_transports));

	/* Clear previous enabled list */
	list_for_each_entry(conf, &dev->configs, list_item) {
		list_for_each_entry(f, &conf->enabled_functions, enabled_list)
			f->android_dev = NULL;
		INIT_LIST_HEAD(&conf->enabled_functions);
	}

	b = strim(buf);

	while (b) {
		conf_str = strsep(&b, ":");
		if (conf_str) {
			/* If the next not equal to the head , take it */
			if (curr_conf->next != &dev->configs)
				conf = list_entry(curr_conf->next,
						  struct android_configuration,
						  list_item);
			else
				conf = alloc_android_config(dev);

			curr_conf = curr_conf->next;
		}

		while (conf_str) {
			name = strsep(&conf_str, ",");
			if (name) {
				err = android_enable_function(dev, conf, name);
				if (err)
					pr_err("android_usb: Cannot enable %s",
						name);
			}
		}
	}

	/* Free uneeded configurations if exists */
	while (curr_conf->next != &dev->configs) {
		conf = list_entry(curr_conf->next,
				  struct android_configuration, list_item);
		free_android_config(dev, conf);
	}

	/* Set the factory mode descriptor. */
	device_desc.idVendor = 0x10A9;
	device_desc.idProduct = 0x1104;
	device_desc.bDeviceClass = 0x02;
	device_desc.bDeviceSubClass = 0x00;
	device_desc.bDeviceProtocol = 0x00;

	cdev->desc.idVendor = device_desc.idVendor;
	cdev->desc.idProduct = device_desc.idProduct;
	cdev->desc.bcdDevice = device_desc.bcdDevice;
	cdev->desc.bDeviceClass = device_desc.bDeviceClass;
	cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
	cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;

	msleep(500);
#if 0
	list_for_each_entry(conf, &dev->configs, list_item)
		if (usb_add_config(cdev, &conf->usb_config,
									android_bind_config));

	usb_gadget_connect(cdev->gadget);

	dev->enabled = true;
#else

	list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f, &conf->enabled_functions,
						enabled_list) {
			if (f->enable)
				f->enable(f);
		}

	android_enable(dev);

	dev->enabled = true;
#endif
	return 0;
}

static void pantech_factory_work(struct work_struct *data)
{
	set_factory_mode();
}

static int factory_mode_ctrlrequest(struct usb_composite_dev *cdev,
	const struct usb_ctrlrequest *ctrl){

	int value = -EOPNOTSUPP;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);
	struct usb_request *req = cdev->req;
	_android_dev = cdev_to_android_dev(cdev);



	if(((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
		&&(ctrl->bRequest == 0x70) && (wValue == 1) && (wLength == 0) && (wIndex == 0)){
			//printk("^^^^^^ It's factory driver\n");
			if(is_factory_called >0){
				//printk("^^ is_factory_called >0\n");
				return value;
			}else{
				//printk("^^ is_factory_called == 0\n");
				is_factory_called++;
			}
			if(is_factory_mode == 0){
				schedule_delayed_work(&factory_work, msecs_to_jiffies(10));
				value = 0;
				if(value >= 0){
					int rc;

					req->zero = value < wLength;
					req->length = value;
					//printk("^^^^^^ It's factory driver2\n");
					rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
					if(rc < 0)
						printk(KERN_ERR "%s setup response queue error\n", __func__);
				}

			}else{

			}
	}
#ifdef FEATURE_PANTECH_MS_OS_COMPATIBLE
		else if(ctrl->bRequest == 1 && (ctrl->bRequestType & USB_DIR_IN) && (wIndex == 4 || wIndex == 5)){
					if(p_pantech_ext_config_desc){
						value = (wLength < p_pantech_ext_config_desc->desc.header.dwLength ?
								wLength : p_pantech_ext_config_desc->desc.header.dwLength);
						memcpy(cdev->req->buf, &p_pantech_ext_config_desc->desc, value);
						//printk(KERN_ERR "%s : length[%d] p_length[%d]\n", __func__, value, p_pantech_ext_config_desc->desc.header.dwLength);
	}else{
						//printk(KERN_ERR "%s : wlength[%d] \n", __func__, wLength);
						//printk(KERN_ERR "%s : p_pantech_ext_config_desc->desc.header.dwLength [%d] \n", __func__, pantech_ext_config_desc_list[0].desc.header.dwLength );
						//printk(KERN_ERR "%s : p_pantech_ext_config_desc->desc.fundtion[0].compatibleID  [%s] \n", __func__, pantech_ext_config_desc_list[0].desc.function[0].compatibleID );
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
                    		if(pantech_cdrom_enabled)
                        {
                            if(function_is_enabled(_android_dev , "adb"))
                            {
        						value = (wLength < pantech_ext_config_desc_list[5].desc.header.dwLength ?
        								wLength : pantech_ext_config_desc_list[5].desc.header.dwLength);
        						memcpy(cdev->req->buf, &pantech_ext_config_desc_list[5].desc, value);
                            }
                            else
                            {
        						value = (wLength < pantech_ext_config_desc_list[4].desc.header.dwLength ?
        								wLength : pantech_ext_config_desc_list[4].desc.header.dwLength);
        						memcpy(cdev->req->buf, &pantech_ext_config_desc_list[4].desc, value);
                            }
                        }
                        else
                        {
#endif
                        	// Resolved MTP YB issue in WinXP if the device was composite, 2013.01.23, Lee, Younhee
                            if(function_is_enabled(_android_dev , "adb"))
                            {
        						value = (wLength < pantech_ext_config_desc_list[2].desc.header.dwLength ?
        								wLength : pantech_ext_config_desc_list[2].desc.header.dwLength);
        						memcpy(cdev->req->buf, &pantech_ext_config_desc_list[2].desc, value);
                            }
                            else
                            {
						value = (wLength < pantech_ext_config_desc_list[0].desc.header.dwLength ?
								wLength : pantech_ext_config_desc_list[0].desc.header.dwLength);


						memcpy(cdev->req->buf, &pantech_ext_config_desc_list[0].desc, value);
						//printk(KERN_ERR "%s : length[%d]\n", __func__, value);
					}
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
                        }
#endif

					}
					if (value >= 0) {
						int rc;
						req->zero = value < wLength;
						req->length = value;
						rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
						if (rc < 0)
							printk(KERN_ERR "%s setup response queue error\n", __func__);
					}

		}
#endif
	else{
		strings_dev[STRING_PRODUCT_IDX].s = product_string;
		DBG(cdev, "^^^^^^ It's not factory driver\n");
	}

	return value;
}
#endif

static ssize_t enable_store(struct device *pdev, struct device_attribute *attr,
			    const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	struct android_usb_function *f;
	struct android_configuration *conf;
	int enabled = 0;
	static DEFINE_RATELIMIT_STATE(rl, 10*HZ, 1);

	if (!cdev)
		return -ENODEV;

	mutex_lock(&dev->mutex);

	sscanf(buff, "%d", &enabled);
	if (enabled && !dev->enabled) {
		/*
		 * Update values in composite driver's copy of
		 * device descriptor.
		 */
		cdev->desc.idVendor = device_desc.idVendor;
		cdev->desc.idProduct = device_desc.idProduct;
		cdev->desc.bcdDevice = device_desc.bcdDevice;

#ifdef CONFIG_PANTECH_ANDROID_MTP
		if(device_desc.idVendor == 0x05c6){ 	/* Qualcomm */
			device_desc.bDeviceClass = 0x00;
			device_desc.bDeviceSubClass = 0x00;
			device_desc.bDeviceProtocol = 0x00;

		}else{ 					/* Pantech */
			if(function_is_enabled(dev,"mtp")){
				if(function_is_enabled(dev,"serial")){
					/* lgu+ pc data mode, pst mode */
					device_desc.bDeviceClass = 0x02;
					device_desc.bDeviceSubClass = 0x00;
					device_desc.bDeviceProtocol = 0x00;
				}else if(function_is_enabled(dev,"adb")){
					/* for mtp + adb */
					device_desc.bDeviceClass = 0x00;
					device_desc.bDeviceSubClass = 0x00;
					device_desc.bDeviceProtocol = 0x00;
				}else{
					/* for mtp only */
					device_desc.bDeviceClass = 0xff;
					device_desc.bDeviceSubClass = 0xff;
					device_desc.bDeviceProtocol = 0x00;
				}
			}else if(function_is_enabled(dev,"serial")){
				/* pantech composition, modem added */
				device_desc.bDeviceClass = 0x02;
				device_desc.bDeviceSubClass = 0x00;
				device_desc.bDeviceProtocol = 0x00;
			}else if(function_is_enabled(dev,"rndis")){
				if(function_is_enabled(dev,"diag")){
					/* rndis + extra functions(diag,adb) */
					device_desc.bDeviceClass = 0x02;
					device_desc.bDeviceSubClass = 0x00;
					device_desc.bDeviceProtocol = 0x00;
				}else{
					/* rndis only */
					device_desc.bDeviceClass = 0xef;
					device_desc.bDeviceSubClass = 0x02;
					device_desc.bDeviceProtocol = 0x01;
				}
			}else{
				/* default */
				device_desc.bDeviceClass = 0x00;
				device_desc.bDeviceSubClass = 0x00;
				device_desc.bDeviceProtocol = 0x00;
			}
		}
#endif
		cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
		list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f, &conf->enabled_functions,
						enabled_list) {
				if (f->enable)
					f->enable(f);
			}
		android_enable(dev);
		dev->enabled = true;
#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
		is_factory_called= 0;
#endif
	} else if (!enabled && dev->enabled) {
		android_disable(dev);
		list_for_each_entry(conf, &dev->configs, list_item)
			list_for_each_entry(f, &conf->enabled_functions,
						enabled_list) {
				if (f->disable)
					f->disable(f);
			}
		dev->enabled = false;
	} else if (__ratelimit(&rl)) {
		pr_err("android_usb: already %s\n",
				dev->enabled ? "enabled" : "disabled");
	}

	mutex_unlock(&dev->mutex);

	return size;
}

#ifdef CONFIG_PANTECH_USB_BLOCKING_MDMSTATE
static ssize_t usb_mdm_control_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "usb_mdm_control_show :: is_mdm_usb_control_enabled = %d\n", is_mdm_usb_control_enabled);
}


static ssize_t usb_mdm_control_store(struct device *pdev,struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	struct android_dev *dev = _android_dev;
	char *disconnected[2] = { "USB_STATE=DISCONNECTED", NULL };

	sscanf(buf, "%d", &value);

	is_mdm_usb_control_enabled = value;

	if(is_mdm_usb_control_enabled){
		printk("usb_mdm_mode enable! : USB disconnection mode!\n");
		kobject_uevent_env(&dev->dev->kobj, KOBJ_CHANGE, disconnected);

	}else{
		printk("usb_mdm_mode disable! : USB connection mode!\n");
	}

	return size;

}
int get_pantech_mdm_state(void)
{
	return is_mdm_usb_control_enabled;
}
EXPORT_SYMBOL(get_pantech_mdm_state);
#endif

static ssize_t pm_qos_show(struct device *pdev,
			   struct device_attribute *attr, char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%s\n", dev->pm_qos);
}

static ssize_t pm_qos_store(struct device *pdev,
			   struct device_attribute *attr,
			   const char *buff, size_t size)
{
	struct android_dev *dev = dev_get_drvdata(pdev);

	strlcpy(dev->pm_qos, buff, sizeof(dev->pm_qos));

	return size;
}

static ssize_t state_show(struct device *pdev, struct device_attribute *attr,
			   char *buf)
{
	struct android_dev *dev = dev_get_drvdata(pdev);
	struct usb_composite_dev *cdev = dev->cdev;
	char *state = "DISCONNECTED";
	unsigned long flags;

	if (!cdev)
		goto out;

	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config)
		state = "CONFIGURED";
	else if (dev->connected)
		state = "CONNECTED";
	spin_unlock_irqrestore(&cdev->lock, flags);
out:
	return snprintf(buf, PAGE_SIZE, "%s\n", state);
}

#ifdef CONFIG_PANTECH_ANDROID_USB
ushort getVendorID(void ){
	return device_desc.idVendor;
}
#endif

#define DESCRIPTOR_ATTR(field, format_string)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return snprintf(buf, PAGE_SIZE,					\
			format_string, device_desc.field);		\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	int value;							\
	if (sscanf(buf, format_string, &value) == 1) {			\
		device_desc.field = value;				\
		return size;						\
	}								\
	return -1;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);

#define DESCRIPTOR_STRING_ATTR(field, buffer)				\
static ssize_t								\
field ## _show(struct device *dev, struct device_attribute *attr,	\
		char *buf)						\
{									\
	return snprintf(buf, PAGE_SIZE, "%s", buffer);			\
}									\
static ssize_t								\
field ## _store(struct device *dev, struct device_attribute *attr,	\
		const char *buf, size_t size)				\
{									\
	if (size >= sizeof(buffer))					\
		return -EINVAL;						\
	strlcpy(buffer, buf, sizeof(buffer));				\
	strim(buffer);							\
	return size;							\
}									\
static DEVICE_ATTR(field, S_IRUGO | S_IWUSR, field ## _show, field ## _store);


DESCRIPTOR_ATTR(idVendor, "%04x\n")
DESCRIPTOR_ATTR(idProduct, "%04x\n")
DESCRIPTOR_ATTR(bcdDevice, "%04x\n")
DESCRIPTOR_ATTR(bDeviceClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceSubClass, "%d\n")
DESCRIPTOR_ATTR(bDeviceProtocol, "%d\n")
DESCRIPTOR_STRING_ATTR(iManufacturer, manufacturer_string)
DESCRIPTOR_STRING_ATTR(iProduct, product_string)
DESCRIPTOR_STRING_ATTR(iSerial, serial_string)

static DEVICE_ATTR(functions, S_IRUGO | S_IWUSR, functions_show,
						 functions_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);
#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
static DEVICE_ATTR(factory_mode, S_IRUGO | S_IWUSR, factory_mode_show, factory_mode_store);
#endif
static DEVICE_ATTR(pm_qos, S_IRUGO | S_IWUSR,
		pm_qos_show, pm_qos_store);
static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);
static DEVICE_ATTR(remote_wakeup, S_IRUGO | S_IWUSR,
		remote_wakeup_show, remote_wakeup_store);

#ifdef CONFIG_PANTECH_USB_BLOCKING_MDMSTATE
static DEVICE_ATTR(usb_mdm_control, S_IRUGO | S_IWUSR, usb_mdm_control_show, usb_mdm_control_store);
#endif
static struct device_attribute *android_usb_attributes[] = {
	&dev_attr_idVendor,
	&dev_attr_idProduct,
	&dev_attr_bcdDevice,
	&dev_attr_bDeviceClass,
	&dev_attr_bDeviceSubClass,
	&dev_attr_bDeviceProtocol,
	&dev_attr_iManufacturer,
	&dev_attr_iProduct,
	&dev_attr_iSerial,
	&dev_attr_functions,
	&dev_attr_enable,
	&dev_attr_pm_qos,
#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
	&dev_attr_factory_mode,
#endif
#ifdef CONFIG_PANTECH_USB_BLOCKING_MDMSTATE
	&dev_attr_usb_mdm_control,
#endif
	&dev_attr_state,
	&dev_attr_remote_wakeup,
	NULL
};

#ifdef CONFIG_PANTECH_USB_TUNE_SIGNALING_PARAM
extern void pantech_set_transceiver_parameter(int type, u32 value);
extern u32 pantech_get_transceiver_parameter(int type);

static ssize_t hs_dc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	value = pantech_get_transceiver_parameter(0);
	return sprintf(buf, "%d", value);
}

static ssize_t hs_dc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	pantech_set_transceiver_parameter(0, value);
	return size;
}

static DEVICE_ATTR(hs_dc, S_IRUGO | S_IWUSR, hs_dc_show, hs_dc_store);

static ssize_t rf_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	value = pantech_get_transceiver_parameter(1);
	return sprintf(buf, "%d", value);
}

static ssize_t rf_time_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	pantech_set_transceiver_parameter(1, value);
	return size;
}

static DEVICE_ATTR(rf_time, S_IRUGO | S_IWUSR, rf_time_show, rf_time_store);

static ssize_t pre_emphasis_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	value = pantech_get_transceiver_parameter(2);
	return sprintf(buf, "%d", value);
}

static ssize_t pre_emphasis_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	pantech_set_transceiver_parameter(2, value);
	return size;
}

static DEVICE_ATTR(pre_emphasis, S_IRUGO | S_IWUSR, pre_emphasis_show, pre_emphasis_store);

static ssize_t impedance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	value = pantech_get_transceiver_parameter(3);
	return sprintf(buf, "%d", value);
}

static ssize_t impedance_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	pantech_set_transceiver_parameter(3, value);
	return size;
}

static DEVICE_ATTR(impedance, S_IRUGO | S_IWUSR, impedance_show, impedance_store);

static struct attribute *android_pantech_usb_control_attrs[] = {
	&dev_attr_hs_dc.attr,
	&dev_attr_rf_time.attr,
	&dev_attr_pre_emphasis.attr,
	&dev_attr_impedance.attr,
  NULL,
};

static struct attribute_group android_pantech_usb_control_attr_grp = {
  .attrs = android_pantech_usb_control_attrs,
};
#endif

/*-------------------------------------------------------------------------*/
/* Composite driver */

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = cdev_to_android_dev(c->cdev);
	int ret = 0;

	ret = android_bind_enabled_functions(dev, c);
	if (ret)
		return ret;

	return 0;
}

static void android_unbind_config(struct usb_configuration *c)
{
	struct android_dev *dev = cdev_to_android_dev(c->cdev);

	android_unbind_enabled_functions(dev, c);
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev;
	struct usb_gadget	*gadget = cdev->gadget;
	struct android_configuration *conf;
	int			gcnum, id, ret;

	/* Bind to the last android_dev that was probed */
	dev = list_entry(android_dev_list.prev, struct android_dev, list_item);

	dev->cdev = cdev;

	/*
	 * Start disconnected. Userspace will connect the gadget once
	 * it is done configuring the functions.
	 */
	usb_gadget_disconnect(gadget);

	/* Init the supported functions only once, on the first android_dev */
	if (android_dev_count == 1) {
		ret = android_init_functions(dev->functions, cdev);
		if (ret)
			return ret;
	}

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	/* Default strings - should be updated by userspace */
	strlcpy(manufacturer_string, "Android",
		sizeof(manufacturer_string) - 1);
	strlcpy(product_string, "Android", sizeof(product_string) - 1);
	strlcpy(serial_string, "0123456789ABCDEF", sizeof(serial_string) - 1);

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	if (gadget_is_otg(cdev->gadget))
		list_for_each_entry(conf, &dev->configs, list_item)
			conf->usb_config.descriptors = otg_desc;

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	return 0;
}

static int android_usb_unbind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = cdev_to_android_dev(cdev);

	manufacturer_string[0] = '\0';
	product_string[0] = '\0';
	serial_string[0] = '0';
	cancel_work_sync(&dev->work);
	android_cleanup_functions(dev->functions);
	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.unbind		= android_usb_unbind,
	.max_speed	= USB_SPEED_SUPER
};

static int
android_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *c)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct android_dev		*dev = cdev_to_android_dev(cdev);
	struct usb_request		*req = cdev->req;
	struct android_usb_function	*f;
	struct android_configuration	*conf;
	int value = -EOPNOTSUPP;
	unsigned long flags;
#ifdef FEATURE_PANTECH_MODS
	u16 wIndex = le16_to_cpu(c->wIndex);
	u16 wValue = le16_to_cpu(c->wValue);
	u16 wLength = le16_to_cpu(c->wLength);
#endif
	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = 0;
	gadget->ep0->driver_data = cdev;


#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
	value = factory_mode_ctrlrequest(cdev, c);
	if(value < 0)
#endif
#ifdef FEATURE_PANTECH_MODS
{
	if( (wValue == 0x3EE)  && (wIndex == 0) &&  (wLength == 18)){

		list_for_each_entry(conf, &dev->configs, list_item) {
			list_for_each_entry(f, &conf->enabled_functions, enabled_list){
				if (f->ctrlrequest) {
					value = f->ctrlrequest(f, cdev, c);
					if (value >= 0)
						break;
				}
			}
		}
	}
#endif
	list_for_each_entry(conf, &dev->configs, list_item)
		list_for_each_entry(f, &conf->enabled_functions, enabled_list)
			if (f->ctrlrequest) {
				value = f->ctrlrequest(f, cdev, c);
				if (value >= 0)
					break;
			}
#ifdef FEATURE_PANTECH_MODS
}
#endif
	/* Special case the accessory function.
	 * It needs to handle control requests before it is enabled.
	 */
	if (value < 0)
		value = acc_ctrlrequest(cdev, c);

	if (value < 0)
		value = composite_setup(gadget, c);

	spin_lock_irqsave(&cdev->lock, flags);
	if (!dev->connected) {
		dev->connected = 1;
		schedule_work(&dev->work);
	} else if (c->bRequest == USB_REQ_SET_CONFIGURATION &&
						cdev->config) {
		schedule_work(&dev->work);
	}
	spin_unlock_irqrestore(&cdev->lock, flags);

	return value;
}

static void android_disconnect(struct usb_gadget *gadget)
{
	struct usb_composite_dev *cdev = get_gadget_data(gadget);
	struct android_dev *dev = cdev_to_android_dev(cdev);
	unsigned long flags;

	composite_disconnect(gadget);
	/* accessory HID support can be active while the
	   accessory function is not actually enabled,
	   so we need to inform it when we are disconnected.
	 */
	acc_disconnect();

	spin_lock_irqsave(&cdev->lock, flags);
	dev->connected = 0;
	schedule_work(&dev->work);
	spin_unlock_irqrestore(&cdev->lock, flags);
}

static int android_create_device(struct android_dev *dev, u8 usb_core_id)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;
	char device_node_name[ANDROID_DEVICE_NODE_NAME_LENGTH];
	int err;

	/*
	 * The primary usb core should always have usb_core_id=0, since
	 * Android user space is currently interested in android0 events.
	 */
	snprintf(device_node_name, ANDROID_DEVICE_NODE_NAME_LENGTH,
		 "android%d", usb_core_id);
	dev->dev = device_create(android_class, NULL,
					MKDEV(0, 0), NULL, device_node_name);
	if (IS_ERR(dev->dev))
		return PTR_ERR(dev->dev);

	dev_set_drvdata(dev->dev, dev);

	while ((attr = *attrs++)) {
		err = device_create_file(dev->dev, attr);
		if (err) {
			device_destroy(android_class, dev->dev->devt);
			return err;
		}
	}
	return 0;
}

static void android_destroy_device(struct android_dev *dev)
{
	struct device_attribute **attrs = android_usb_attributes;
	struct device_attribute *attr;

	while ((attr = *attrs++))
		device_remove_file(dev->dev, attr);
	device_destroy(android_class, dev->dev->devt);
}

static struct android_dev *cdev_to_android_dev(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = NULL;

	/* Find the android dev from the list */
	list_for_each_entry(dev, &android_dev_list, list_item) {
		if (dev->cdev == cdev)
			break;
	}

	return dev;
}

static struct android_configuration *alloc_android_config
						(struct android_dev *dev)
{
	struct android_configuration *conf;

	conf = kzalloc(sizeof(*conf), GFP_KERNEL);
	if (!conf) {
		pr_err("%s(): Failed to alloc memory for android conf\n",
			__func__);
		return ERR_PTR(-ENOMEM);
	}

	dev->configs_num++;
	conf->usb_config.label = dev->name;
	conf->usb_config.unbind = android_unbind_config;
	conf->usb_config.bConfigurationValue = dev->configs_num;

	INIT_LIST_HEAD(&conf->enabled_functions);

	list_add_tail(&conf->list_item, &dev->configs);

	return conf;
}

static void free_android_config(struct android_dev *dev,
			     struct android_configuration *conf)
{
	list_del(&conf->list_item);
	dev->configs_num--;
	kfree(conf);
}

static int __devinit android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *android_dev;
	int ret = 0;

	if (!android_class) {
		android_class = class_create(THIS_MODULE, "android_usb");
		if (IS_ERR(android_class))
			return PTR_ERR(android_class);
	}

	android_dev = kzalloc(sizeof(*android_dev), GFP_KERNEL);
	if (!android_dev) {
		pr_err("%s(): Failed to alloc memory for android_dev\n",
			__func__);
		ret = -ENOMEM;
		goto err_alloc;
	}

	android_dev->name = pdev->name;
	android_dev->disable_depth = 1;
	android_dev->functions = supported_functions;
	android_dev->configs_num = 0;
	INIT_LIST_HEAD(&android_dev->configs);
	INIT_WORK(&android_dev->work, android_work);
	mutex_init(&android_dev->mutex);

	android_dev->pdata = pdata;

#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
	_android_dev = android_dev;
#endif

	list_add_tail(&android_dev->list_item, &android_dev_list);
	android_dev_count++;

	if (pdata)
		composite_driver.usb_core_id = pdata->usb_core_id;
	else
		composite_driver.usb_core_id = 0; /*To backward compatibility*/

	ret = android_create_device(android_dev, composite_driver.usb_core_id);
	if (ret) {
		pr_err("%s(): android_create_device failed\n", __func__);
		goto err_dev;
	}

	ret = usb_composite_probe(&android_usb_driver, android_bind);
	if (ret) {
		pr_err("%s(): Failed to register android "
				 "composite driver\n", __func__);
		goto err_probe;
	}

	/* pm qos request to prevent apps idle power collapse */
	if (pdata && pdata->swfi_latency)
		pm_qos_add_request(&android_dev->pm_qos_req_dma,
			PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
	strlcpy(android_dev->pm_qos, "high", sizeof(android_dev->pm_qos));
#ifdef CONFIG_PANTECH_USB_TUNE_SIGNALING_PARAM
	ret = sysfs_create_group(&pdev->dev.kobj, &android_pantech_usb_control_attr_grp);
	if(ret < 0){
		printk(KERN_ERR "[%s]sysfs_create_group error\n", __func__);
		return -1;
	}
#endif

	return ret;
err_probe:
	android_destroy_device(android_dev);
err_dev:
	list_del(&android_dev->list_item);
	android_dev_count--;
	kfree(android_dev);
err_alloc:
	if (list_empty(&android_dev_list)) {
		class_destroy(android_class);
		android_class = NULL;
	}
	return ret;
}

static int android_remove(struct platform_device *pdev)
{
	struct android_dev *dev = NULL;
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	int usb_core_id = 0;

	if (pdata)
		usb_core_id = pdata->usb_core_id;

	/* Find the android dev from the list */
	list_for_each_entry(dev, &android_dev_list, list_item) {
		if (!dev->pdata)
			break; /*To backward compatibility*/
		if (dev->pdata->usb_core_id == usb_core_id)
			break;
	}

	if (dev) {
		android_destroy_device(dev);
		if (pdata && pdata->swfi_latency)
			pm_qos_remove_request(&dev->pm_qos_req_dma);
		list_del(&dev->list_item);
		android_dev_count--;
		kfree(dev);
	}

	if (list_empty(&android_dev_list)) {
		class_destroy(android_class);
		android_class = NULL;
		usb_composite_unregister(&android_usb_driver);
	}

	return 0;
}

static const struct platform_device_id android_id_table[] __devinitconst = {
	{
		.name = "android_usb",
	},
	{
		.name = "android_usb_hsic",
	},
};

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb"},
	.probe = android_probe,
	.remove = android_remove,
	.id_table = android_id_table,
};

static int __init init(void)
{
	int ret;

#ifdef CONFIG_PANTECH_ANDROID_FACTORY_MODE
	INIT_DELAYED_WORK(&factory_work, pantech_factory_work);
#endif

	/* Override composite driver functions */
	composite_driver.setup = android_setup;
	composite_driver.disconnect = android_disconnect;

	INIT_LIST_HEAD(&android_dev_list);
	android_dev_count = 0;

	ret = platform_driver_register(&android_platform_driver);
	if (ret) {
		pr_err("%s(): Failed to register android"
				 "platform driver\n", __func__);
	}

	return ret;
}
module_init(init);

static void __exit cleanup(void)
{
	platform_driver_unregister(&android_platform_driver);
}
module_exit(cleanup);
