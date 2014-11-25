How to Build
    1. Get Toolchain (arm-eabi-4.7) and install
       (Visit android git server or codesourcery)

    2. modify below lines in build_kernel.sh and run
	  1) modify below lines
		$ export ARCH=arm
		$ export PATH=$(pwd)/../../toolchain_arm-eabi-4.7/arm-eabi-4.7/bin:$PATH
		$ export CROSS_COMPILE=arm-eabi-
      2) run build_kernel.sh
		$ ./build_kernel.sh

    3.Output Files
      -	kernel : kernel/zImaze
      -	module : kernel/drivers/*/*.ko

	4.wlan download
	  - https://www.codeaurora.org/cgit/quic/la/platform/vendor/qcom-opensource/wlan/prima/commit/?h=LNX.LA.2.7.3&id=5ecbee2a3988bbb8be0c7cf013b1efa1c995de59
	