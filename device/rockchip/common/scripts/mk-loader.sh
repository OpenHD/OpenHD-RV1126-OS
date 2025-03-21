#!/bin/bash -e

UEFI_DIR=uefi/edk2-platforms/Platform/Rockchip/DeviceTree
MAKE_CMD="./make.sh CROSS_COMPILE=$CROSS_COMPILE"

do_build_uefi()
{
	if [ "$RK_CHIP" != rk3588 -o ! -d uefi ]; then
		echo "UEFI not supported!"
		return 0
	fi

	run_command cp "$RK_KERNEL_DTB" $UEFI_DIR/$RK_CHIP.dtb
	run_command cd uefi
	run_command $MAKE_CMD $RK_UBOOT_CFG
}

build_uefi()
{
	if [ "$RK_CHIP" != rk3588 -o ! -d uefi ]; then
		echo "UEFI not supported!"
		return 1
	fi

	echo "============Start building uefi============"
	echo "Copy kernel dtb $RK_KERNEL_DTB to $UEFI_DIR/$RK_CHIP.dtb"
	echo "========================================="
	if [ ! -f "$RK_KERNEL_DTB" ]; then
		echo "$RK_KERNEL_DTB not exists!"
		return 1
	fi

	do_build_uefi $@

	finish_build
}

do_build_uboot()
{
	check_config RK_UBOOT_CFG || return 0

	ARGS="$RK_UBOOT_OPTS \
		${RK_UBOOT_TRUST_INI:+../rkbin/RKTRUST/$RK_UBOOT_TRUST_INI} \
		${RK_UBOOT_SPL_INI:+../rkbin/RKBOOT/$RK_UBOOT_SPL_INI}"

	if [ "$RK_SECURITY" ]; then
		if [ -z "$RK_SECURITY_OTP_DEBUG" ]; then
			ARGS="$ARGS --burn-key-hash"
		fi

		if [ "$RK_AB_UPDATE" ]; then
			DEFAULT_IMAGES=boot
		else
			DEFAULT_IMAGES="boot recovery"
		fi

		for p in ${1:-$DEFAULT_IMAGES}; do
			ARGS="--${p}_img $SDK_DIR/u-boot/$p.img $ARGS"
		done
	fi

	run_command cd u-boot
	run_command $MAKE_CMD \
		$RK_UBOOT_CFG $RK_UBOOT_CFG_FRAGMENTS $(echo $ARGS)
	run_command cd ..
}

build_uboot()
{
	check_config RK_UBOOT_CFG || return 0

	echo "============Start building uboot============"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_CFG"
	echo "========================================="

	rm -f u-boot/*.bin u-boot/*.img

	do_build_uboot $@

	if [ "$RK_SECURITY" ];then
		ln -rsf u-boot/boot.img "$RK_FIRMWARE_DIR"
		[ "$RK_AB_UPDATE" ] || \
			ln -rsf u-boot/recovery.img "$RK_FIRMWARE_DIR"
	fi

	LOADER="$(echo u-boot/*_loader_*v*.bin | head -1)"
	SPL="$(echo u-boot/*_loader_spl.bin | head -1)"
	ln -rsf "${LOADER:-$SPL}" "$RK_FIRMWARE_DIR"/MiniLoaderAll.bin

	ln -rsf u-boot/uboot.img "$RK_FIRMWARE_DIR"
	[ ! -e u-boot/trust.img ] || \
		ln -rsf u-boot/trust.img "$RK_FIRMWARE_DIR"

	finish_build
}

do_build_spl()
{
	check_config RK_UBOOT_SPL_CFG || return 0

	run_command cd u-boot
	run_command $MAKE_CMD $RK_UBOOT_SPL_CFG
	run_command $MAKE_CMD --spl
	run_command cd ..
}

build_spl()
{
	check_config RK_UBOOT_SPL_CFG || return 0

	echo "============Start building spl============"
	echo "TARGET_SPL_CONFIG=$RK_UBOOT_SPL_CFG"
	echo "========================================="

	rm -f u-boot/*spl.bin

	do_build_spl $@

	SPL="$(echo u-boot/*_loader_spl.bin | head -1)"
	ln -rsf "$SPL" "$RK_FIRMWARE_DIR/MiniLoaderAll.bin"

	finish_build
}

# Hooks

usage_hook()
{
	echo "loader             - build loader (uboot|spl)"
	echo "uboot              - build u-boot"
	echo "spl                - build spl"
	echo "uefi               - build uefi"
}

clean_hook()
{
	make -C u-boot distclean
}

BUILD_CMDS="loader uboot spl uefi"
build_hook()
{
	if [ "$2" = cmds ]; then
		echo -e "\e[35mCommands of building $1:\e[0m"
		DRY_RUN=1 build_hook $1
		return 0
	fi

	TARGET="$1"
	shift

	if [ "$TARGET" = loader ]; then
		if [ "$RK_UBOOT_SPL_CFG" ]; then
			TARGET=spl
		else
			TARGET=uboot
		fi
	fi

	case "$TARGET" in
		uboot | spl | uefi)
			FUNC=${DRY_RUN:+do_}build_$TARGET
			$FUNC $@
			;;
		*) usage ;;
	esac
}

build_hook_dry()
{
	build_hook "$1" cmds
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

build_hook ${@:-loader}
