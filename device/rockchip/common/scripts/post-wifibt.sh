#!/bin/bash -e

source "${POST_HELPER:-$(dirname "$(realpath "$0")")/../post-hooks/post-helper}"

echo "Building Wifi/BT module and firmwares..."

"$SCRIPTS_DIR/mk-wifibt.sh" "$TARGET_DIR"
