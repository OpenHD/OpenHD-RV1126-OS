#!/bin/bash -e

source "${POST_HELPER:-$(dirname "$(realpath "$0")")/../post-hooks/post-helper}"

LOCALE="$TARGET_DIR/etc/default/locale"

[ -n "$RK_ROOTFS_LANG" ] || return 0

echo "Setting LANG to $RK_ROOTFS_LANG..."

if [ -e "$LOCALE" ]; then
	sed -i "/\<LANG\>/d" "$LOCALE"
	echo "LANG=$RK_ROOTFS_LANG" >> "$LOCALE"
else
	mkdir -p "$TARGET_DIR/etc/profile.d"
	echo "export LANG=$RK_ROOTFS_LANG" > \
		"$TARGET_DIR/etc/profile.d/lang.sh"
fi
