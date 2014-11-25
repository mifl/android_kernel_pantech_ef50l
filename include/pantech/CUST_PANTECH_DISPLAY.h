#ifndef F_SKYDISP_FRAMEWORK_FEATURE
#define F_SKYDISP_FRAMEWORK_FEATURE

/*
 * ##1199 Test Menu, Turn on/off LCD.
 * Front assay ¢®¨¡???¢¥???¢ç¡§?¢®? LCD, Touch, ¡§?¡§?¡§?¢®?, side ?¢®¨¡ ???? ????¡À?¢®¨¡?¢ç???¡§?¢®? ?¢®¢¯¡§?¡§?¡§¢®????? ???????
 * ?????¨£ OFF??? ??¢®¨¡? LCD ???? ???¢®¨¡???¢®¨¡? ?????¨£ ON?? ??????? ?????
 */
#define CONFIG_F_SKYDISP_TURN_ONOFF_LCD
 
 /* 
  * LCD ON/OFF For Factory process command.
  * 
  * KERNEL : not used
  * USER   : used
  */ 
#define F_SKYDISP_FACTORY_PROCESS_CMD

/* 
 * LCD related ATCMD(Check that LCD ON or OFF) 
 * KERNEL : not used 
 * USER   : used 
 */ 
#define CONFIG_F_SKYDISP_ATCMD

/* 
 * 2012.05.23, kkcho 
 * Macro for  CABC_CTRL on Sharp-LCD 
 * KERNEL : used     
 * USER   : used 
 */ 
#define CONFIG_F_SKYDISP_CABC_CTRL

/* 
 * 2012.05.23, kkcho 
 * Macro for  CABC_Tunning on Sharp-LCD 
 * KERNEL : used     
 * USER   : used 
 * This macro will enable the LCD_tuning only. 
 * M1 : by LCDtestmenu in the ##### -> Do Not Use this style
 * M2 : by adb shell command 
 */ 
//#define CONFIG_F_SKYDISP_CE_TUNING_M2

/* 
 * DO     : Reduce the time displaying empty screen when switching 'LK offline charging image' to 'offilen charing app image'. 
 * NOTE   : Although related source is located in android framework(Therefore 'KERNEL : not used, USER : used'), 
 *          binary image is contained in kernel binary image(boot.img). 
 * KERNEL : not used 
 * USER   : used 
 */ 
#define CONFIG_F_SKYDISP_REDUCE_DISPLAYING_EMPTY_SCREEN_WHEN_SWITCHING_LK_TO_OFFLINECHARGING_APP 

/* 120923 p14198
 * DO     : Add int gr_set_bl_level(unsigned char bl_level)
 *          From now on, recovery mode use this function.
 */
#define CONFIG_F_SKYDISP_ADD_GR_SET_BL_LEVEL

/* 120923 p14198
 * DO     : Set bl level of recovery mode
 */
#ifdef CONFIG_F_SKYDISP_ADD_GR_SET_BL_LEVEL
	#define CONFIG_F_SKYDISP_SET_BL_LEVEL_OF_RECOVERY_MODE
#endif

/* 121017 p14198
 * DO     : Block load_565rle_image(), When I didn't blcok the code, device couldn't boot(kernel panic occured.).
 * file   : /system/core/init/logo.c
 *     @@@ NOTE @@@
 *      Eventhough define is here, logo.c is compiled with kernel compile and then boot.img is created.
 */
#define CONFIG_F_SKYDISP_BLOCK_BOOT_LOGO_IN_INIT_PROCESS

/* 121107 p14682
 * DO     : CASE patch Web brower  bug fixed
 * file   : /libs/hwui/OpenGLRenderer.cpp b/libs/hwui/OpenGLRenderer.cpp
 */
#define CONFIG_F_SKYDISP_BROWER_BREAK_BUF_FIXED

/*
 * 2012.11.19 lived
 * Do     : WFD Interface for ITEC EF48/49/50
 * KERNEL : not used 
 * USER   : used 
 */
#if defined(T_EF48S) || defined(T_EF49K) || defined(T_EF50L)
#define F_SKYDISP_WFD_API_FOR_ITEC
#endif

/* 20121212 p14198 : Used in KERNEL and FRAMWORK
 * We can see some noise(for example, when rotate 1080p video). That's why blocked blt mode(writeback).
 * Tuned rotater ratio for blocking MDP underrun.
 * Refer to QC CASE #01034021 [EF51]IOMMU page fault occur when rotating the video repeatedly.
 *                  #01044917 [EF51]Underrun occur when rotating 1080p portrate video to 270 degree.
 */
#define CONFIG_F_SKYDISP_QCBUGFIX_BLOCK_BLT_MODE_AND_TUNE_ROTATER_RATIO


/*****************************************************
* 20130722, p16603, Kim.HG
* Where defined : kernel, sky_ctrl_drv
* PLM  :
* Case : 
* Description : Add sharpness control
* Related files :
* ./kernel/drivers/video/msm/mdp4_overlay.c
* ./kernel/drivers/video/msm/msm_fb.*
* ./vendor/pantech/frameworks/sky_ctrl_drv/java/com/pantech/test/Sky_ctrl_drv.java
* ./vendor/pantech/frameworks/sky_ctrl_drv/jni/sky_ctrl_drv.cpp
* added files    :no added
********************************************************/
#define PANTECH_LCD_SHARPNESS_CTRL

#if defined(T_EF50L) || defined(T_EF51L) || defined(T_EF52L)
/*
 * UVS Service related, Implement checkFramebufferUpdate() API.
 * Only for LGU+
 * KERNEL : not used
 * USER   : used
*/
#define CONFIG_F_SKYDISP_UVS
#endif

 /*****************************************************
* owner  : p12452          
* date    : 2014.05.14
* PLM    : 
* Case  :       
* Description : DRM contents is not shown to external device , (ex HDMI , MiraCast) 
* kernel : none
* user    : used
******************************************************/
#define CONFIG_F_SKYDISP_DRM_ISNOT_SHOWN_TO_MIRACAST

 /*****************************************************
* owner  : p13832
* date    : 2013.08.19
* PLM    :
* Case  :
* Description : it's for lcd backlight
* kernel : used
* user    : used
******************************************************/
#define CONFIG_F_SKYDISP_BACKLIGHT_CMDS_CTL

 /*****************************************************
* owner  : p13832      
* date    : 2014.06.11
* PLM    : 
* Case  :   01561678 
* Description : the hwui cache error(image corruption)
* kernel : none
* user    : used
* TODO : (JB patch)
******************************************************/
#define GOOGLE_BUG_FIX_GRAPHICS_CORRUPTION_BY_HWUI_CACHES

 /*****************************************************
* owner  : p14974
* date    : 2014.07.17
* PLM    : S 233
* Case  :   01624060
* Description : hwui: Always enable the scissor while composing layer
* kernel : none
* user    : used
******************************************************/
#if defined(T_EF51S) || defined(T_EF51K) || defined(T_EF51L)
#define CONFIG_F_SKYDISP_STOP_POPUP_BUG_SUPPLEMENT
#endif

/*****************************************************
* owner  : p14974
* date    : 2014.07.28
* PLM    : L 228
* Case  :   01642671
* Description : gralloc: Move getAllocator in unlock
* gralloc: Perform function to return custom stride
* gralloc: Perform func to return stride and height
* kernel : none
* user    : used
******************************************************/
#define CONFIG_F_SKYDISP_YOUTUBE_BLACK_SCREEN_BUFFIX

 /*****************************************************
* owner  : p13832     
* date    : 2014.07.16
* PLM    : EF63S_3508
* Case  :   01628717  
* Description : Fix the texture ID reuse issue in HWUI
                      Issue: When the layer of previous frame is destroyed, it doesn't clear the
			 texture id in mBoundTextures[mTextureUnit], so in the next frame, if
			 glGenTexture returns same texture ID of the previous frame,
			 the new texture is not bound.

			 CRs-fixed: 671736
* kernel : none
* user    : used
* TODO : 
******************************************************/
#define GOOGLE_BUGFIX_HWUI_CLEAR_ID

/*****************************************************
* owner  : p14974
* date    : 2014.08.06
* PLM    : EF51L_336
* Case  : 01656628 
* Description : Fix MDP UNDERRUN when playing 1080p downscaling
* kernel : none
* user    : used
* TODO :
******************************************************/
#define CONFIG_F_SKYDISP_FIX_UNDERRUN_1080P_DOWNSCALING


//+US1-CF1
//Google Patches (HWUI , GUI, UI)
#define FEATURE_HWUI_GUI_GOOGLE_PATCHES
//-US1-CF1

#endif  /* SKY_FRAMEWORK_FEATURE */
