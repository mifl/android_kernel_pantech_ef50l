/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sony_incell.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>

#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE  0

#define NOP()	do {asm volatile ("NOP");} while(0);
#define DELAY_3NS() do { \
    asm volatile ("NOP"); \
    asm volatile ("NOP"); \
    asm volatile ("NOP");} while(0);

#define LCD_DEBUG_MSG
#ifdef LCD_DEBUG_MSG
#define ENTER_FUNC()        printk(KERN_INFO "[SKY_LCD] +%s \n", __FUNCTION__);
#define EXIT_FUNC()         printk(KERN_INFO "[SKY_LCD] -%s \n", __FUNCTION__);
#define ENTER_FUNC2()       printk(KERN_ERR "[SKY_LCD] +%s\n", __FUNCTION__);
#define EXIT_FUNC2()        printk(KERN_ERR "[SKY_LCD] -%s\n", __FUNCTION__);
#define PRINT(fmt, args...) printk(KERN_INFO fmt, ##args)
#define DEBUG_EN 1
#else
#define PRINT(fmt, args...)
#define ENTER_FUNC2()
#define EXIT_FUNC2()
#define ENTER_FUNC()
#define EXIT_FUNC()
#define DEBUG_EN 0
#endif

#ifdef CONFIG_F_SKYDISP_SILENT_BOOT
#include <mach/pantech_sys.h>
unsigned int is_silent_boot_mode, is_backlight_on_before_reset, is_silent_boot_mode_n_bl_off;
#endif

#if ((defined(CONFIG_SKY_EF52S_BOARD)||defined(CONFIG_SKY_EF52K_BOARD)) && (CONFIG_BOARD_VER >= CONFIG_TP20)) || (defined(CONFIG_SKY_EF52L_BOARD)&& (CONFIG_BOARD_VER >= CONFIG_TP10)) || (defined(CONFIG_SKY_EF52W_BOARD)&& (CONFIG_BOARD_VER >= CONFIG_WS10))
#define FEATURE_SKYDISP_BOOT_ANI_SKIP_BUG_FIX // 20130103, kkcho, Bug-Fix :no display boot_ani at some-device
#endif

//extern int gpio43, gpio16, gpio24; /* gpio43 :reset, gpio16:lcd bl */
#ifdef FEATURE_SKYDISP_BOOT_ANI_SKIP_BUG_FIX
static int lcd_on_skip_during_bootup =0; 
#define LCD_VCI_EN 28
#else
#define LCD_VCI_EN 14
#endif

extern int gpio_lcd_bl_en,gpio_lcd_bl_ctl;

static int prev_bl_level = 0;

static struct msm_panel_common_pdata *mipi_sony_pdata;

static struct dsi_buf sony_tx_buf;
static struct dsi_buf sony_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
   struct mutex lcd_mutex;
};

static struct lcd_state_type sony_state = { 0, };

char wrdisbv[2]        = {0x51, 0x00};
char wrctrld[2]         = {0x53, 0x2C};
char wrcbc_on[2]     = {0x55, 0x03};
char wrcbc_off[2]     = {0x55, 0x00};

char sleep_out[3]     = {0x11, 0x00};
char sleep_in[2]       = {0x10, 0x00};
char disp_on[3]        = {0x29, 0x00};
char disp_off[2]       = {0x28, 0x00};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_off), disp_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 140, sizeof(sleep_in), sleep_in}
};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on}
};

static struct dsi_cmd_desc sony_display_init_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(sleep_out), sleep_out},
#if 1		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc_on), wrcbc_on},			

#endif 
};

static struct dsi_cmd_desc sony_display_cabc_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc_on), wrcbc_on},	
};

static struct dsi_cmd_desc sony_display_cabc_bl_set_cmds[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv}
};     

//#define BOOT_TOUCH_RESET
#define PANTECH_LCD_REQUEST_GPIO_TOUCHRESET //shkwak 20130215, to prevent gpio warning oops logs

#ifdef BOOT_TOUCH_RESET
#include <linux/gpio.h>
#include <mach/gpio.h>
int touch_init = false;
#define TOUCH_RST_GPIO	43
#ifdef PANTECH_LCD_REQUEST_GPIO_TOUCHRESET 
static u16 touch_reset_gpio = TOUCH_RST_GPIO;
static int gpio_init = false;
#endif
#endif

char mipi_sony_backlight_tbl[17] =
{0, 35, 40, 50, 60, 70, 80, 90, 100, 115, 130, 145, 160, 175, 190, 205, 220};
//05.03 modify for current consumption
//{0, 35, 40, 55, 70, 85,  100, 115, 130, 145, 160, 175, 190, 205, 220, 235, 255};

#ifdef CONFIG_FB_PANTECH_MIPI_SONY_CMD_HD_PANEL // 20121226, kkcho, for ##1199 LCD Front-Test
int mipi_sony_force_lcd_on(void)
{
	struct msm_fb_data_type *mfd;
	struct fb_info *info; 

	struct dcs_cmd_req cmdreq;

	ENTER_FUNC2();

	info = registered_fb[0];
	mfd = (struct msm_fb_data_type *)info->par;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
    	mutex_lock(&sony_state.lcd_mutex);	

	if (sony_state.disp_initialized == false) {
		cmdreq.cmds = sony_display_init_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_init_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		sony_state.disp_initialized = true;
	}

	cmdreq.cmds = sony_display_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	sony_state.disp_on = true;

	cmdreq.cmds = sony_display_cabc_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	wrdisbv[1] = 0;
	cmdreq.cmds = sony_display_cabc_bl_set_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_bl_set_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	mutex_unlock(&sony_state.lcd_mutex);	

#if defined(CONFIG_F_SKYDISP_SILENT_BOOT) && defined(CONFIG_PANTECH_ERR_CRASH_LOGGING)
    // In case of reset when silentboot mode(doing boot), only when is_silent_boot_mode_n_bl_off == 0, do it.
    // If always do it, display silentboot image eventhough reset occur when backlight off.
    if(is_silent_boot_mode_n_bl_off == 0)
	pantech_sys_reset_backlight_flag_set(true);	
#endif


#ifdef BOOT_TOUCH_RESET
	//gpio_direction_output(TOUCH_RST_GPIO, 1);
	gpio_set_value(TOUCH_RST_GPIO, GPIO_LOW_VALUE);
	msleep(50);
	gpio_set_value(TOUCH_RST_GPIO, GPIO_HIGH_VALUE);

#endif
	
	mipi_dsi_set_tear_on(mfd);

	EXIT_FUNC2();
	return 0;
}

int mipi_sony_force_lcd_off(void)
{
	struct msm_fb_data_type *mfd;
	struct fb_info *info; 
	struct dcs_cmd_req cmdreq;

	ENTER_FUNC2();

	info = registered_fb[0];
	mfd = (struct msm_fb_data_type *)info->par;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
    mutex_lock(&sony_state.lcd_mutex);	
	
    if (sony_state.disp_on == true) {
		//mipi_set_tx_power_mode(0);
		cmdreq.cmds = sony_display_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);

        sony_state.disp_on = false;
        sony_state.disp_initialized = false;

#if defined(CONFIG_F_SKYDISP_SILENT_BOOT) && defined(CONFIG_PANTECH_ERR_CRASH_LOGGING)
	 pantech_sys_reset_backlight_flag_set(false);	
        is_silent_boot_mode_n_bl_off = 0;
#endif		
        //mipi_set_tx_power_mode(1);	
    }

    mutex_unlock(&sony_state.lcd_mutex);	

#ifdef FEATURE_RENESAS_BL_CTRL_CHG
    wrdisbv[1] = 0;
#endif

    EXIT_FUNC2();
    return 0;
}
#endif

//extern void LCD_gpio_set_vci_control(int on);
static int mipi_sony_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
	struct dcs_cmd_req cmdreq;
#endif
	ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

#ifdef FEATURE_SKYDISP_BOOT_ANI_SKIP_BUG_FIX
	if (!lcd_on_skip_during_bootup)
	{
		lcd_on_skip_during_bootup = true;
		sony_state.disp_initialized = true;
		sony_state.disp_on = true;
		goto out;
	}
#endif

	mutex_lock(&sony_state.lcd_mutex);	

	if(!gpio_get_value_cansleep(gpio_lcd_bl_en))
		 gpio_set_value_cansleep(gpio_lcd_bl_en, GPIO_HIGH_VALUE);


	if (sony_state.disp_initialized == false) {
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
		cmdreq.cmds = sony_display_init_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_init_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

#else
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_init_cmds,
				ARRAY_SIZE(sony_display_init_cmds));
#endif
		sony_state.disp_initialized = true;
	}

#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
		cmdreq.cmds = sony_display_on_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_on_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

#else
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_on_cmds,
			ARRAY_SIZE(sony_display_on_cmds));
#endif
	sony_state.disp_on = true;

#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
	cmdreq.cmds = sony_display_cabc_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);
#else	
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_on_cmds,
				ARRAY_SIZE(sony_display_cabc_on_cmds));
#endif 

	mutex_unlock(&sony_state.lcd_mutex);	

#ifdef FEATURE_SKYDISP_BOOT_ANI_SKIP_BUG_FIX
	out:
#endif
#if defined(CONFIG_F_SKYDISP_SILENT_BOOT) && defined(CONFIG_PANTECH_ERR_CRASH_LOGGING)
	// In case of reset when silentboot mode(doing boot), only when is_silent_boot_mode_n_bl_off == 0, do it.
	// If always do it, display silentboot image eventhough reset occur when backlight off.
	if(is_silent_boot_mode_n_bl_off == 0)
		pantech_sys_reset_backlight_flag_set(true);	
#endif

#ifdef BOOT_TOUCH_RESET // touch reset code
#ifdef PANTECH_LCD_REQUEST_GPIO_TOUCHRESET
	if (touch_init == false) {
		pr_info("[LIVED] %s touch_init=%d, gpio_init=%d\n", __func__, touch_init, gpio_init);
		if (gpio_init == false) {
			if (gpio_request(touch_reset_gpio, "TOUCH_RESET_GPIO") != 0) {
				pr_err("[LCD]%s: fail to request touch_reset_gpio=%d, touch_init=%d, gpio_init=%d\n",
						__func__, touch_reset_gpio, touch_init, gpio_init);
			}

			gpio_init = true;
		}
		gpio_direction_output(TOUCH_RST_GPIO, 1);
		gpio_set_value(TOUCH_RST_GPIO, GPIO_LOW_VALUE);
		msleep(50);
		gpio_set_value(TOUCH_RST_GPIO, GPIO_HIGH_VALUE);
		touch_init = true;
	}
#else
	if (touch_init == false) {
		gpio_direction_output(TOUCH_RST_GPIO, 1);
		gpio_set_value(TOUCH_RST_GPIO, GPIO_LOW_VALUE);
		msleep(50);
		gpio_set_value(TOUCH_RST_GPIO, GPIO_HIGH_VALUE);
		touch_init = true;
	}
#endif
#endif	

#ifdef CONFIG_F_SKYDISP_EF52_KK_TEARING_WORKAROUND
	mfd->ibuf.vsync_enable = TRUE;
#endif

	EXIT_FUNC2();
	return 0;
}

static int mipi_sony_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
	struct dcs_cmd_req cmdreq;
#endif

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
    mutex_lock(&sony_state.lcd_mutex);	
	
    if (sony_state.disp_on == true) {
        sony_state.disp_on = false;
        sony_state.disp_initialized = false;		
        //mipi_set_tx_power_mode(0);
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
		cmdreq.cmds = sony_display_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
#else
        mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_off_cmds,
                ARRAY_SIZE(sony_display_off_cmds));
#endif

#if defined(CONFIG_F_SKYDISP_SILENT_BOOT) && defined(CONFIG_PANTECH_ERR_CRASH_LOGGING)
	pantech_sys_reset_backlight_flag_set(false);	
        is_silent_boot_mode_n_bl_off = 0;
#endif		
        //mipi_set_tx_power_mode(1);	
    }
	
#ifdef CONFIG_F_SKYDISP_EF52_KK_TEARING_WORKAROUND
    mfd->ibuf.vsync_enable = FALSE;
#endif

#ifdef FEATURE_RENESAS_BL_CTRL_CHG
    wrdisbv[1] = 0;

	if(gpio_get_value_cansleep(gpio_lcd_bl_en))
      		gpio_set_value_cansleep(gpio_lcd_bl_en, GPIO_LOW_VALUE);
#endif

    mutex_unlock(&sony_state.lcd_mutex);	

    EXIT_FUNC2();
    return 0;
}

void mipi_cabc_lcd_bl_init(int bl_level)
{
#ifdef CONFIG_FB_PANTECH_MIPI_SONY_CMD_HD_PANEL
	struct dcs_cmd_req cmdreq;
#endif
	//printk(KERN_ERR"[SKY_LCD]mipi_cabc_lcd_bl_init\n");
	//dump_stack();

	if (bl_level >= 1 && bl_level <= 16) {
		wrdisbv[1] = mipi_sony_backlight_tbl[bl_level];
#ifdef CONFIG_FB_PANTECH_MIPI_SONY_CMD_HD_PANEL
		mutex_lock(&sony_state.lcd_mutex);
		mipi_set_tx_power_mode(0);
		cmdreq.cmds = sony_display_cabc_bl_set_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_bl_set_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		mipi_set_tx_power_mode(1);	
		mutex_unlock(&sony_state.lcd_mutex);
#endif		
		gpio_set_value_cansleep(gpio_lcd_bl_ctl, GPIO_HIGH_VALUE);
		gpio_set_value_cansleep(gpio_lcd_bl_en, GPIO_HIGH_VALUE);
	}

#ifdef BOOT_TOUCH_RESET
	// shkwak 20111112, ##1199 reset need touch reset.
	if(touch_init == true)
		touch_init = false;
#endif

}

#ifdef PANTECH_LCD_MIPI_CONTROL_CHANGE
extern int msmfb_early_suspend_get_state(void);
#endif

static void mipi_sony_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;	
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
	struct dcs_cmd_req cmdreq;
#endif

#ifdef CONFIG_F_SKYDISP_SKIP_BLSET_WITH_EFS_ERASE
	mfd->bl_set_first_skip =0;
#endif

#ifndef FEATURE_RENESAS_BL_CTRL_CHG
	if (prev_bl_level == mfd->bl_level)
		return;
#endif

#ifdef PANTECH_LCD_MIPI_CONTROL_CHANGE
	if (sony_state.disp_on == false){
		printk(KERN_INFO"[#######SKY_LCD#######] bl-set return during disp-off \n");
		return;
	}

	if (msmfb_early_suspend_get_state()){ 
	  	printk(KERN_INFO"[#######SKY_LCD#######] bl-set return during suspend \n");
		return;
	}
	
#endif

	ENTER_FUNC2();
	
	bl_level=mfd->bl_level;
	printk(KERN_INFO"[SKY_LCD]mipi_sony_set_backlight prev_bl_level = %d bl_level =%d \n",prev_bl_level,mfd->bl_level);
	
#ifdef CONFIG_F_SKYDISP_SILENT_BOOT
	if (is_silent_boot_mode_n_bl_off == 1) {
	    printk(KERN_ERR"DONOT set backlight because this time is silentboot mode.\n");
	    return;
	}
#endif
	mutex_lock(&sony_state.lcd_mutex);	

	wrdisbv[1] = mipi_sony_backlight_tbl[bl_level];

	if(!gpio_get_value_cansleep(gpio_lcd_bl_en))
		 gpio_set_value_cansleep(gpio_lcd_bl_en, GPIO_HIGH_VALUE);

	mipi_set_tx_power_mode(0);
	
#ifdef MIPI_CMDSTX_CHANGE_TO_CMDLISTPUT
	cmdreq.cmds = sony_display_cabc_bl_set_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_bl_set_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#else
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_bl_set_cmds,
			ARRAY_SIZE(sony_display_cabc_bl_set_cmds));
#endif
	prev_bl_level = mfd->bl_level;	
	mipi_set_tx_power_mode(1);

	mutex_unlock(&sony_state.lcd_mutex);	

	EXIT_FUNC2();  
}

#if (0) //def CONFIG_FB_PANTECH_MIPI_SONY_CMD_HD_PANEL  // kkcho, 20121213, Only Command_MODE applied
static int only_cabc_val_durnig_bootani =0;
#endif
void cabc_control(struct msm_fb_data_type *mfd, int state)
{
#ifdef CONFIG_FB_PANTECH_MIPI_SONY_CMD_HD_PANEL  // kkcho, 20121213, Only Command_MODE applied
	struct dcs_cmd_req cmdreq;

	printk(KERN_INFO"[LCD] %s() state:%d\n", __func__, state);
	
	if(state == true)
	    wrcbc_on[1] = 0; // CABC OFF
	else
	    wrcbc_on[1] = 3; // 00 off 01 ui 02 still 03 movie

	if(sony_state.disp_initialized){
		mutex_lock(&sony_state.lcd_mutex);
		mipi_set_tx_power_mode(0);
		cmdreq.cmds = sony_display_cabc_on_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_on_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		mipi_set_tx_power_mode(1);	
		mutex_unlock(&sony_state.lcd_mutex);
#if (0)
		if (!is_silent_boot_mode_n_bl_off && !only_cabc_val_durnig_bootani){
			mutex_lock(&sony_state.lcd_mutex);
			mipi_set_tx_power_mode(0);
			wrdisbv[1] = 0x40;  // First backlight_value at boot-animation, must set : same lk_value!
			cmdreq.cmds = sony_display_cabc_bl_set_cmds;
			cmdreq.cmds_cnt = ARRAY_SIZE(sony_display_cabc_bl_set_cmds);
			cmdreq.flags = CMD_REQ_COMMIT;
			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			mipi_dsi_cmdlist_put(&cmdreq);
			mipi_set_tx_power_mode(1);	
			mutex_unlock(&sony_state.lcd_mutex);
			only_cabc_val_durnig_bootani =1;
		}
#endif
	}
#else
	printk(KERN_INFO"[LCD] %s+, do nothing\n", __func__);
#endif	
}

void SKY_LCD_CE_CASE_SET(struct msm_fb_data_type *mfd, uint32_t CEcase)
{
    printk(KERN_INFO"[LCD] %s+, do nothing\n", __func__);	
}

static int __devinit mipi_sony_lcd_probe(struct platform_device *pdev)
{
    if (pdev->id == 0) {
        mipi_sony_pdata = pdev->dev.platform_data;
		return 0;
	}
       mutex_init(&sony_state.lcd_mutex);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_sony_lcd_probe,
	.driver = {
		.name   = "mipi_sony_insell",
	},
};

static struct msm_fb_panel_data sony_panel_data = {
       .on             = mipi_sony_lcd_on,
       .off            = mipi_sony_lcd_off,
       .set_backlight  = mipi_sony_set_backlight,
};

static int ch_used[3];

int mipi_sony_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_sony_insell", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	sony_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &sony_panel_data,
		sizeof(sony_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_sony_lcd_init(void)
{
    ENTER_FUNC2();

#if defined(CONFIG_F_SKYDISP_SILENT_BOOT) && defined(CONFIG_PANTECH_ERR_CRASH_LOGGING)
    is_silent_boot_mode = pantech_sys_rst_is_silent_boot_mode(); 
    is_backlight_on_before_reset = pantech_sys_reset_backlight_flag_get();	
    
    if(is_silent_boot_mode == 1 && is_backlight_on_before_reset == 0)
    {
        printk("This time is silent boot mode.\n");
        is_silent_boot_mode_n_bl_off = 1;
    }
    else
        printk("This time is NOT silent boot mode.\n");
#endif 

    sony_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&sony_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&sony_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}

module_init(mipi_sony_lcd_init);
