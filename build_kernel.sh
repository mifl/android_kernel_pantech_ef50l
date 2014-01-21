#!/bin/bash
###############################################################################
#
#                           Kernel Build Script 
#
###############################################################################
# 2011-10-24 effectivesky : modified
# 2010-12-29 allydrop     : created
###############################################################################
##############################################################################
# set toolchain
##############################################################################
export ARCH=arm
export CROSS_COMPILE=$PWD/../../../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
export LINUX_BIN_PATH=$PWD/obj
rm -rf $LINUX_BIN_PATH
CMD_V_LOG_FILE=$PWD/KERNEL_build.log
rm -rf $CMD_V_LOG_FILE

##############################################################################
# make zImage
##############################################################################
mkdir -p ./obj/KERNEL_OBJ/
make O=./obj/KERNEL_OBJ msm8960-perf_defconfig
make -j4 O=./obj/KERNEL_OBJ 2>&1 | tee $CMD_V_LOG_FILE

##############################################################################
# Copy Kernel Image
##############################################################################
cp -f ./obj/KERNEL_OBJ/arch/arm/boot/zImage .

