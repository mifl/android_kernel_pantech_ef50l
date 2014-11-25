#!/bin/bash
###############################################################################
#
#                            EF51L Board Rev Select Script 
#
###############################################################################
# 2011-06-16 choiseulkee : created
###############################################################################
LOCAL_V_BOARD_VER=
while [ -z $LOCAL_V_BOARD_VER ]
do
	echo "**************************************************************************"
	echo "*               $CMD_V_UPPER_PRJ_NAME Select Board Version                      "
	echo "**************************************************************************"
	echo " 1. TP20"
	echo " 2. TP10"
#	echo " 2. WS20"
#	echo " 3. WS10"
	echo " 0. exit"
	echo "**************************************************************************"
	echo .
	echo -n "Which Board Version would you select ? [0] "

	read LOCAL_V_BOARD_VER

	case $LOCAL_V_BOARD_VER in
	1)
		export CMD_V_BOARD_VER=tp20
		;;
	2)
		export CMD_V_BOARD_VER=tp10
		;;
	3)
		export CMD_V_BOARD_VER=ws20
		;;
	4)
		export CMD_V_BOARD_VER=ws10
		;;
	0)
		;;
	*)
		echo "Your selected Board Ver is invalid number"
		LOCAL_V_BOARD_VER=
	esac
	echo
done
