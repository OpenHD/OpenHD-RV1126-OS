cmd_spl/drivers/misc/rv1126-secure-otp.o := /github/OpenHD-RV1126-OS/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-rockchip631-linux-gnueabihf-gcc -Wp,-MD,spl/drivers/misc/.rv1126-secure-otp.o.d  -nostdinc -isystem /github/OpenHD-RV1126-OS/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/../lib/gcc/arm-linux-gnueabihf/6.3.1/include -Iinclude    -I./arch/arm/include -include ./include/linux/kconfig.h -D__KERNEL__ -D__UBOOT__ -DCONFIG_SPL_BUILD -D__ASSEMBLY__ -g -D__ARM__ -Wa,-mimplicit-it=always -mthumb -mthumb-interwork -mabi=aapcs-linux -mno-unaligned-access -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe -march=armv7-a -D__LINUX_ARM_ARCH__=7 -I./arch/arm/mach-rockchip/include   -c -o spl/drivers/misc/rv1126-secure-otp.o drivers/misc/rv1126-secure-otp.S

source_spl/drivers/misc/rv1126-secure-otp.o := drivers/misc/rv1126-secure-otp.S

deps_spl/drivers/misc/rv1126-secure-otp.o := \

spl/drivers/misc/rv1126-secure-otp.o: $(deps_spl/drivers/misc/rv1126-secure-otp.o)

$(deps_spl/drivers/misc/rv1126-secure-otp.o):
