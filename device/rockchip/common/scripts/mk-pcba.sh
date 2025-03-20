#!/bin/bash -e

usage_hook()
{
	echo "pcba               - build PCBA"
}

clean_hook()
{
	check_config RK_PCBA_CFG || return 0
	rm -rf buildroot/output/$RK_PCBA_CFG
	rm -rf "$RK_OUTDIR/pcba"
}

BUILD_CMDS="pcba"
build_hook()
{
	check_config RK_PCBA_CFG || return 0

	echo "==========Start building pcba(buildroot)=========="
	echo "TARGET_PCBA_CONFIG=$RK_PCBA_CFG"
	echo "========================================"

	DST_DIR="$RK_OUTDIR/pcba"

	/usr/bin/time -f "you take %E to build pcba(buildroot)" \
		"$SCRIPTS_DIR/mk-buildroot.sh" $RK_PCBA_CFG "$DST_DIR"

	/usr/bin/time -f "you take %E to pack pcba image" \
		"$SCRIPTS_DIR/mk-ramdisk.sh" "$DST_DIR/rootfs.cpio.gz" \
		"$DST_DIR/pcba.img"
	ln -rsf "$DST_DIR/pcba.img" "$RK_FIRMWARE_DIR"

	finish_build build_pcba
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

build_hook $@
