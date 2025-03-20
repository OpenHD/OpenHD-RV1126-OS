#!/bin/bash -e

KERNELS=$(ls | grep kernel- || true)

do_build()
{
	check_config RK_KERNEL_DTS_NAME RK_KERNEL_CFG RK_BOOT_IMG || return 0

	run_command $KMAKE $RK_KERNEL_CFG $RK_KERNEL_CFG_FRAGMENTS

	if [ "$1" = modules ]; then
		run_command $KMAKE modules
		return 0
	fi

	run_command $KMAKE "$RK_KERNEL_DTS_NAME.img"

	# The FIT image for initrd would be packed in rootfs stage
	if [ -n "$RK_BOOT_FIT_ITS" ] && [ -z "$RK_ROOTFS_INITRD" ]; then
		run_command "$SCRIPTS_DIR/mk-fitimage.sh" \
			"kernel/$RK_BOOT_IMG" "$RK_BOOT_FIT_ITS" \
			"$RK_KERNEL_IMG"
	fi
}

# Hooks

usage_hook()
{
	for k in $KERNELS; do
		echo "${k/-4\.4/-4.4 }        - build kernel ${k#kernel-}"
	done

	echo "kernel             - build kernel"
	echo "modules            - build kernel modules"
	echo "linux-headers      - build linux-headers"
}

clean_hook()
{
	make -C kernel distclean
}

INIT_CMDS="$KERNELS"
init_hook()
{
	sed -i "s/^\(RK_KERNEL_VERSION=\).*/\1\"${1#kernel-}\"/" \
		"$RK_CONFIG"
}

PRE_BUILD_CMDS="default"
pre_build_hook()
{
	# Fallback to current kernel
	RK_KERNEL_VERSION=${RK_KERNEL_VERSION:-$(kernel_version)}

	# Fallback to 5.10 kernel
	RK_KERNEL_VERSION=${RK_KERNEL_VERSION:-5.10}

	sed -i "s/^\(RK_KERNEL_VERSION=\).*/\1\"$RK_KERNEL_VERSION\"/" \
		"$RK_CONFIG"

	[ "$(kernel_version)" != "$RK_KERNEL_VERSION" ] || return 0

	# Update kernel
	KERNEL_DIR=kernel-$RK_KERNEL_VERSION
	echo "switching to $KERNEL_DIR"
	if [ ! -d "$KERNEL_DIR" ]; then
		echo "$KERNEL_DIR not exist!"
		exit 1
	fi

	rm -rf kernel
	ln -rsf $KERNEL_DIR kernel
}

BUILD_CMDS="$KERNELS kernel modules"
build_hook()
{
	check_config RK_KERNEL_DTS_NAME RK_KERNEL_CFG RK_BOOT_IMG || return 0

	if [ "$2" = cmds ]; then
		echo -e "\e[35mCommands of building $1:\e[0m"
		echo "export CROSS_COMPILE=$CROSS_COMPILE"
		DRY_RUN=1 do_build $1
		return 0
	fi

	echo "============Start building $1============"
	echo "TARGET_KERNEL_VERSION =$RK_KERNEL_VERSION"
	echo "TARGET_KERNEL_ARCH   =$RK_KERNEL_ARCH"
	echo "TARGET_KERNEL_CONFIG =$RK_KERNEL_CFG"
	echo "TARGET_KERNEL_CONFIG_FRAGMENTS =$RK_KERNEL_CFG_FRAGMENTS"
	echo "TARGET_KERNEL_DTS    =$RK_KERNEL_DTS_NAME"
	echo "=========================================="

	if [ "$1" = modules ]; then
		do_build $@
		shift
		finish_build build_modules
		exit 0
	fi

	do_build $@

	ln -rsf "kernel/$RK_BOOT_IMG" "$RK_FIRMWARE_DIR/boot.img"

	[ -z "$RK_SECURITY" ] || cp "$RK_FIRMWARE_DIR/boot.img" u-boot/

	"$SCRIPTS_DIR/check-power-domain.sh"

	finish_build build_kernel
}

build_hook_dry()
{
	build_hook "$1" cmds
}

POST_BUILD_CMDS="linux-headers"
post_build_hook()
{
	check_config RK_KERNEL_DTS_NAME RK_KERNEL_CFG RK_BOOT_IMG || return 0

	OUTPUT_DIR="${2:-"$RK_OUTDIR/linux-headers"}"

	echo "Saving linux-headers to $OUTPUT_DIR"

	rm -rf "$OUTPUT_DIR"
	mkdir -p "$OUTPUT_DIR"

	cd kernel
	{
		# Based on kernel/scripts/package/builddeb
		find . arch/$RK_KERNEL_ARCH -maxdepth 1 -name Makefile\*
		find include scripts -type f -o -type l
		find arch/$RK_KERNEL_ARCH -name module.lds -o -name Kbuild.platforms -o -name Platform
		find $(find arch/$RK_KERNEL_ARCH -name include -o -name scripts -type d) -type f
		find arch/$RK_KERNEL_ARCH/include Module.symvers include scripts -type f
	} > "$OUTPUT_DIR/.linux-headers-files"
	tar -c -f - -C . -T "$OUTPUT_DIR/.linux-headers-files" | \
		tar -xf - -C "$OUTPUT_DIR"
	cp .config "$OUTPUT_DIR"
	cd -
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

case "${1:-kernel}" in
	kernel-*) init_hook $@ ;&
	kernel | modules)
		pre_build_hook
		build_hook ${@:-kernel}
		;;
	linux-headers) post_build_hook $@ ;;
	*) usage ;;
esac
