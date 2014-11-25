#ifndef __CUST_LGU_CP_LINUX_H
#define __CUST_LGU_CP_LINUX_H

/* =============================================================================
FILE: cust_lgu_cp_linux.h

Copyright (c) 2010 by PANTECH Incorporated.  All Rights Reserved.

ï¿½ï¿½)     FEATURE_LGU_CP_xxx_XXX_001
         Ver.001 : 2011-08-18 : Jake, Lee 
ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ : -------------------------------------------------------------------------

1.	Custfileï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Ú¼ï¿½ï¿½ï¿½ ï¿½Û¼ï¿½ï¿½ï¿½ ï¿½Ö½Ã°ï¿½ ï¿½Ø´ï¿½ ï¿½Úµï¿½ ï¿½ÎºÐ¿ï¿½ featureï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½Û¼ï¿½ï¿½Õ´Ï´ï¿½.
2.	ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½Û¼ï¿½ï¿½Ã¿ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ä±¸ ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ standard spec. ï¿½ï¿½ indexï¿½ï¿½ ï¿½ì¼±ï¿½ï¿½ï¿½ï¿½ ï¿½Û¼ï¿½ï¿½Ï½Ã°ï¿½
3.	ï¿½×¿ï¿½ ï¿½ï¿½Ã»ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ side effect ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ ï¿½Û¼ï¿½ ï¿½Õ´Ï´ï¿½.

============================================================================= */

/*************************************************/
/*                     COMMON                    */
/*************************************************/
#define FEATURE_LGU_CP_COMMON_LOCAL_DB_WITH_QMI
#define FEATURE_LGU_COMMON_DEBUG_SCREEN
#define FEATURE_LGU_CP_COMMON_NVIO_WITH_QMI

/*
    DDMS ï¿½ï¿½ ï¿½ï¿½ï¿½Ø¼ï¿½ radio log È®ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½Ø¼ï¿½ï¿½ï¿½ ï¿½Æ·ï¿½ featureï¿½ï¿½ defineï¿½Ø¾ï¿½ï¿½ï¿½..
*/
/* #define FEATURE_SKY_CP_RADIO_LOG_DDMS */

/*
    PS1 add.
*/
#define FEATURE_LGU_CP_RIL_FW_ANDROID_REQUEST_HNDL_MAX_EVT_ID_FIX
#define FEATURE_LGU_CP_FW_DEDICATED_ANDROID_REQ_BUG_FIX

#define FEATURE_LGU_CP_MANAGER_DAEMON_INTERFACE
#ifdef FEATURE_LGU_CP_MANAGER_DAEMON_INTERFACE
#define FEATURE_LGU_CP_CPMGRIF_QMI_CLIENT
#define FEATURE_LGU_CP_OEM_COMMANDS_WITH_QMI
#define FEATURE_LGU_CP_OEM_QMI_ACCESS
#define FEATURE_LGU_COMMON_TELEPHONY_IF
#endif

#define FEATURE_LGU_CP_MANAGER_GW_DAEMON_INTERFACE
#define FEATURE_LGU_GW_COMMON_TELEPHONY_IF

#define FEATURE_LGU_CP_COMMON_GLOBAL_SD

#define FEATURE_LGU_CP_COMMON_TEST_MODE

#define FEATURE_LGU_CP_SKY_ENG_MENU

#define FEATURE_LGU_CP_COMMON_RSSI

#define FEATURE_LGU_CP_COMON_DEFAULT_SETTINGS

#define FEATURE_LGU_CP_COMMON_MDN_MIN_FIX

#define FEATURE_LGU_CP_COMMON_PLMN_FIX_FOR_CDMA

/* SKY ï¿½Úµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ Interface */
#define FEATURE_LGU_CP_AUTOANSWER_INTERFACE

#define FEATURE_LGU_CP_CARD_POWER_DOWN_NOT_SUPPORT

#define FEATURE_LGU_CP_COMMON_OPERATORS_FOR_3GPP2

#define FEATURE_LGU_CP_COMMON_SUPPORT_DIAG_CALL

#define FEATURE_LGU_CP_COMMON_CLEAR_ALL_CONNECTIONS_WHEN_LPM

#define FEATURE_LGU_CP_COMMON_UPDATE_VOICE_RTE

#define FEATURE_LGU_CP_GUARD_NULL_EXCEPTION_WHEN_PHONE_CHANGE

#define FEATURE_LGU_CP_COMMON_UPDATE_SERVICE_STATE_FOR_DATA_ONLY

#define FEATURE_LGU_CP_COMMON_ROAMING_STATE_FIX

#define FEATURE_LGU_CP_COMMON_ANDROID_OTASP_BLOCK

#define FEATURE_LGU_CP_COMMON_RUDE_WAKEUP

#define FEATURE_LGU_CP_COMMON_CARD_ABSENT_BUG_FIX

#define FEATURE_LGU_CP_COMMON_OPENING_DAY
#ifdef FEATURE_LGU_CP_COMMON_OPENING_DAY
#define FEATURE_SKY_CP_NEW_OPENING_DAY
#define FEATURE_SKY_CP_OEM_PH_EVENT_WITH_QMI /* Deliver CM PH event info via qmi(nas) */
#endif /* FEATURE_LGU_CP_COMMON_OPENING_DAY */

#define FEATURE_SKY_CP_DEBUGGING_LOG_FOR_TIME_SETTING

/*
   Android Factory reset, NV should be initialized default value.
*/
#define FEATURE_LGU_CP_FACTORY_RESET_NV_INIT

#define FEATURE_LGU_CP_LINUX_BUILD_TEST

#define FEATURE_LGU_CP_COMMON_SPN_DEBUG

/*************************************************/
/*                     1X                        */
/*************************************************/

#define FEATURE_LGU_CP_1X_DEFAULT_SETUP

#define FEATURE_LGU_CP_1X_LOCAL_DB

#ifdef FEATURE_LGU_CP_1X_LOCAL_DB
#define FEATURE_LGU_CP_1X_LOCAL_DB_WITH_QMI
#endif /* FEATURE_LGU_CP_1X_LOCAL_DB */

#ifdef FEATURE_LGU_COMMON_DEBUG_SCREEN
#define FEATURE_LGU_CP_1X_DEBUG_SCREEN
#endif /* FEATURE_LGU_COMMON_DEBUG_SCREEN */

#define FEATURE_LGU_CP_OEM_API

#define FEATURE_LGU_CP_1X_OTASP

#define FEATURE_LGU_CP_1X_PRL_VERSION

#define FEATURE_LGU_CP_1X_SYSLOST_MONITOR

#define FEATURE_LGU_CP_1X_TEST_CALL_SO

#define FEATURE_LGU_CP_1X_SWITCH_TO_BURST_DTMF

#define FEATURE_LGU_CP_1X_TEST_SBA_FOR_CNAP

#define FEATURE_LGU_CP_1X_8BIT_CHAR_SUPPORT

#define FEATURE_LGU_CP_1X_REG_STATE_UPDATE

#define FEATURE_LGU_CP_1X_FACTORY_INIT

#define FEATURE_LGU_CP_1X_REJ_DISPLAY

#undef FEATURE_LGU_CP_1X_EXT_DISPLAY_Q_BUGFIX // Remove Feature from EF50L(Fixed by QCT EF50L patch) 12. 07. 24 / kkosu

#define FEATURE_LGU_CP_1X_INCOMING_STATE_CHANGE_SKIP

#define FEATURE_LGU_CP_1X_2ND_CALL_Q_BUG_FIX

#define FEATURE_LGU_CP_SKIP_TOA_CHECK // related to modem Feature : FEATURE_LGU_CP_1X_NUMBERTYPE_FIX

#define FEATURE_LGU_CP_1X_MISSED_CALL_BUG_FIX

#define FEATURE_LGU_CP_1X_HANGUP_REQ_Q_BUGFIX // 12.10.30 kkosu, qcril_qmi_voice.c, avoid ril crash when voice call hangup

/*************************************************/
/*                    EVDO                       */
/*************************************************/
#ifdef FEATURE_LGU_COMMON_DEBUG_SCREEN
#define FEATURE_LGU_CP_EVDO_DEBUG_SCREEN
#define FEATURE_LGU_CP_EVDO_DB_QMI
#endif /* FEATURE_LGU_COMMON_DEBUG_SCREEN */

#define FEATURE_LGU_CP_EVDO_ENG_MODE
#ifdef FEATURE_LGU_CP_EVDO_ENG_MODE
#define FEATURE_LGU_CP_EVDO_REV_SETTING
#define FEATURE_LGU_CP_EVDO_SESSION_RESET
#define FEATURE_LGU_CP_EVDO_ERROR_REASON_CODE_WITH_QMI
#define FEATURE_LGU_CP_EVDO_CHECK_VT_REG_STATUS
#define FEATURE_LGU_CP_EVDO_WIPI_NETWORK_ERROR_CODE  
#define FEATURE_LGU_CP_EVDO_SESSION_CLOSE_NOTY_FOR_VT
#endif

/*************************************************/
/*                 WCDMA/GSM                     */
/*************************************************/
#define FEATURE_LGU_CP_GW_LOCAL_DB

#define FEATURE_LGU_CP_LOCAL_DB_GW_ACCESS_CPMGRIF

#define FEATURE_LGU_CP_GW_LOCAL_DB_WITH_QMI

#define FEATURE_LGU_CP_GW_DEBUG_SCREEN

#define FEATURE_LGU_CP_GW_REJECT_CAUSE_DISPLAY

#define FEATURE_LGU_CP_GW_RSSI_LEVEL

/* QMI  CM system selection preference  */
#define FEATURE_LGU_CP_CM_SYS_SELECTION_PREF

/* UMTS SMS MO PS/CS Domain setting menu support */
#define FEATURE_LGU_CP_SMS_CFG_SET_GW_DOMAIN

/* Phone Operation Mode setting (lpm, offline, online .. ) */
#define FEATURE_LGU_CP_PHONE_OPERATION_MODE

/* Voice Call connection sound event */
#define FEATURE_LGU_CP_GW_CS_CALL_CONNECTION_SND_START

/*
    GW signal value ( setup ind. msg )
*/   
#define FEATURE_LGU_CP_GW_SETUP_IND_SIGNAL_VALUE

/* PLMN network selection  */
#define FEATURE_LGU_CP_GW_NETWORK_PLMN_MANUAL_SELECTION
#ifdef FEATURE_LGU_CP_GW_NETWORK_PLMN_MANUAL_SELECTION
#define FEATURE_LGU_CP_GW_MANUAL_SELECTION_WITH_RAT
#endif/* FEATURE_LGU_CP_GW_NETWORK_PLMN_MANUAL_SELECTION */

/* Limited service */
#define FEATURE_LGU_CP_GW_SERVICE_STATUS_DETAIL_SUBSTATE

/* 
    Network Name
*/
#define FEATURE_LGU_CP_GW_QMI_SYS_INFO_ALWAYS_UPDATE

/* 
   PS only mode, CS reg. state is no service...
*/
#define FEATURE_LGU_CP_GW_SUPPORT_PS_ONLY_MODE

/*
   MccMnc Table
*/
#define FEATURE_LGU_CP_GW_PLMN_TABLE_LIST_SEARCH

#define FEATURE_LGU_CP_GW_GET_MCCMNC_UPDATE_IN_LIMITED_SRV

/*
   network name is not displayed,  bug fix
*/
#define FEATURE_LGU_CP_GW_FIX_OPERATOR_NAME_DISPLAY

#define FEATURE_LGU_CP_GW_INIT_NITZ_INFO_PROPERTY

/*
   invalid code input --> network no response --> ui pop-up remain forever....
*/
#define FEATURE_LGU_CP_GW_USSD_NEWORK_NO_RESPONSE

// centralized_eons_supported feature is undefined.  network manual search list issue
#define FEATURE_LGU_CP_GW_CENTRALIZED_EONS_NOT_SUPPORTED

// USSD response msg. decoding issue fix.
#define FEATURE_LGU_CP_GW_USSD_RESPONSE_MSG_DECODING_BUG_FIX


/*
   À½¿µÁö¿ª¿¡¼­ È£ Á¾·á½Ã ¸ÁÀ¸·ÎºÎÅÍ RRC release¸¦ ¼ö½Å ¹ÞÁö ¸øÇÏ°í
   ÀçÂ÷ ´Ü¸»¿¡¼­ ¹ß½Å ½Ãµµ ÈÄ ¹Ù·Î È£Á¾·á½Ã 30ÃÊ°£ MO PendingµÇ¾î Dial È­¸éÀÌ Áö¼ÓÀûÀ¸·Î º¸ÀÌ´Â Çö»ó ¼öÁ¤
*/
#define FEATURE_SKY_CP_HANGUP_BUG_FIX

/*************************************************/
/*                    LTE                        */
/*************************************************/
#ifdef FEATURE_LGU_COMMON_DEBUG_SCREEN
#define FEATURE_LGU_CP_LTE_DEBUG_SCREEN
#define FEATURE_LGU_CP_LTE_DB_QMI
#endif /* FEATURE_LGU_COMMON_DEBUG_SCREEN */

#define FEATURE_LGU_CP_LTE_PROCESS_REJECT

#define FEATURE_LGU_CP_LTE_ANDSF_CELLID

#define FEATURE_LGU_CP_LTE_TEST_MODE_SETTING

// for VoLTE Barring
#define FEATURE_LGU_CP_COMMON_CM_PH_EVENT
#ifdef FEATURE_LGU_CP_COMMON_CM_PH_EVENT
#define FEATURE_LGU_CP_LTE_MO_DATA_BARRING_NOTIFICATION
#endif

/*************************************************/
/*                    UICC                       */
/*************************************************/
#define FEATURE_LGU_CP_UICC_CARD_CUSTOM_INFO
#define FEATURE_LGU_CP_UICC_CARD_TYPE
#define FEATURE_LGU_CP_UICC_CARD_MODE
#define FEATURE_LGU_CP_UICC_SUPPORT_FOR_ISIM_APPLICATION

#define FEATURE_LGU_CP_UICC_BIP_STATUS
#define FEATURE_LGU_CP_UICC_GET_ATR_QMI
#define FEATURE_LGU_CP_UICC_SUPPORT_NFC
#define FEATURE_LGU_CP_UICC_SUPPORT_AKA
#define FEAUTRE_LGU_CP_UICC_ERROR_FIX
#define FEATURE_LGU_CP_UICC_CHECK_ROAMING_SETTINGS
#define FEATURE_LGU_CP_UICC_STK_RESEND
#define FEATURE_LGU_CP_UICC_UI
#define FEATURE_LGU_CP_UICC_FIXED_QC_PROBLEM_FOR_CARD_STATUS
#define FEATURE_LGU_CP_UICC_CARD_STATUS_PINPUK_RETRY_CNT

/*************************************************/
/*                   LBS(GPS)                    */
/*************************************************/
//EF51L_GPS_PORTING_LINUX

//These are for both of modem and linux
#define FEATURE_SKY_CP_GNSS_INTERFACE
#define FEATURE_LGU_CP_GNSS_XTRA_DL
#define FEATURE_LGU_CP_GNSS_XTRA_DL_TIME
#define FEATURE_LGU_CP_GNSS_NMEA_WRITE

//These are for modem
#define FEATURE_SKY_CP_GNSS_CLIENT_STATE
#define FEATURE_SKY_CP_GNSS_COMMON
#define FEATURE_LGU_CP_GNSS_NETWORK_INFO

//These are for linux
#define FEATURE_SKY_CP_GNSS_FIX_FAIL
#define FEATURE_SKY_CP_GNSS_CONFIGURATION
#define FEATURE_SKY_CP_GNSS_MDM_SETTING
#define FEATURE_LGU_CP_GNSS_TEST_SUPPORT

#endif /* __CUST_LGU_CP_LINUX_H */

