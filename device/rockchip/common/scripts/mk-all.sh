#!/bin/bash -e

BOARD=$(echo $RK_KERNEL_DTS_NAME | tr '[:lower:]' '[:upper:]')

build_all()
{
	echo "============================================"
	echo "TARGET_KERNEL_ARCH=$RK_KERNEL_ARCH"
	echo "TARGET_CHIP_FAMILY=$RK_CHIP_FAMILY"
	echo "TARGET_CHIP=$RK_CHIP"
	echo "TARGET_UBOOT_CONFIG=$RK_UBOOT_CFG"
	echo "TARGET_SPL_CONFIG=$RK_UBOOT_SPL_CFG"
	echo "TARGET_KERNEL_CONFIG=$RK_KERNEL_CFG"
	echo "TARGET_KERNEL_DTS=$RK_KERNEL_DTS_NAME"
	echo "TARGET_BUILDROOT_CONFIG=$RK_BUILDROOT_CFG"
	echo "TARGET_RECOVERY_CONFIG=$RK_RECOVERY_CFG"
	echo "TARGET_PCBA_CONFIG=$RK_PCBA_CFG"
	echo "TARGET_RAMBOOT=$RK_ROOTFS_INITRD"
	echo "============================================"

	rm -rf $RK_FIRMWARE_DIR
	mkdir -p $RK_FIRMWARE_DIR

	# NOTE: On secure boot-up world, if the images build with fit(flattened image tree)
	#       we will build kernel and ramboot firstly,
	#       and then copy images into u-boot to sign the images.
	if [ -z "$RK_SECURITY" ];then
		"$SCRIPTS_DIR/mk-loader.sh" loader
	fi

	"$SCRIPTS_DIR/mk-security.sh" security_check

	"$SCRIPTS_DIR/mk-kernel.sh"
	"$SCRIPTS_DIR/mk-rootfs.sh"
	"$SCRIPTS_DIR/mk-recovery.sh"

	if [ "$RK_SECURITY" ];then
		"$SCRIPTS_DIR/mk-loader.sh" loader
	fi

	finish_build
}

build_save()
{
	shift
	SAVE_DIR="$RK_OUTDIR/$BOARD${1:+/$1}"
	case "$(grep "^ID=" "$RK_OUTDIR/os-release" 2>/dev/null)" in
		ID=buildroot) SAVE_DIR="$SAVE_DIR/BUILDROOT" ;;
		ID=debian) SAVE_DIR="$SAVE_DIR/DEBIAN" ;;
		ID=poky) SAVE_DIR="$SAVE_DIR/YOCTO" ;;
	esac
	[ -n "$1" ] || SAVE_DIR="$SAVE_DIR/$(date  +%Y%m%d_%H%M%S)"
	mkdir -p "$SAVE_DIR"

	echo "Saving into $SAVE_DIR..."

	echo "Saving linux-headers..."
	"$SCRIPTS_DIR/mk-kernel.sh" linux-headers "$SAVE_DIR/linux-headers"

	echo "Saving images..."
	mkdir -p "$SAVE_DIR/kernel"
	cp kernel/.config "$SAVE_DIR/kernel"
	cp kernel/vmlinux "$SAVE_DIR/kernel"

	mkdir -p "$SAVE_DIR/IMAGES/"
	cp "$RK_FIRMWARE_DIR"/* "$SAVE_DIR/IMAGES/"

	echo "Saving patches..."
	mkdir -p "$SAVE_DIR/PATCHES"
	.repo/repo/repo forall -j $(( $CPUS + 1 )) -c \
		"\"$SCRIPTS_DIR/save-patches.sh\" \
		\"$SAVE_DIR/PATCHES/\$REPO_PATH\" \$REPO_PATH \$REPO_LREV"
	install -D -m 0755 "$COMMON_DIR/data/apply-all.sh" "$SAVE_DIR/PATCHES/"

	echo "Saving build info..."
	yes | ${PYTHON3:-python3} .repo/repo/repo manifest -r \
		-o "$SAVE_DIR/manifest.xml"

	cp "$RK_FINAL_ENV" "$RK_CONFIG" "$RK_DEFCONFIG" "$SAVE_DIR/"
	ln -rsf "$RK_CONFIG" "$SAVE_DIR/build_info"

	echo "Saving build logs..."
	cp -r "$RK_LOG_BASE_DIR" "$SAVE_DIR/"

	finish_build
}

build_allsave()
{
	build_all
	"$SCRIPTS_DIR/mk-firmware.sh"
	"$SCRIPTS_DIR/mk-updateimg.sh"
	build_save $@

	finish_build
}

# Hooks

usage_hook()
{
	echo "all                - build all basic image"
	echo "save               - save images and build info"
	echo "allsave            - build all & firmware & updateimg & save"
}

clean_hook()
{
	rm -rf "$RK_OUTDIR"/$BOARD*
}

BUILD_CMDS="all allsave"
build_hook()
{
	case "$1" in
		all) build_all ;;
		allsave) build_allsave $@ ;;
	esac
}

POST_BUILD_CMDS="save"
post_build_hook()
{
	build_save $@
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

case "${1:-allsave}" in
	all) build_all ;;
	allsave) build_allsave $@ ;;
	save) build_save $@ ;;
	*) usage ;;
esac
