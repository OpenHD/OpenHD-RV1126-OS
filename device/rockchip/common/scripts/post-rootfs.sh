#!/bin/bash -e

echo "Executing $(basename "$BASH_SOURCE")..."

# Trigger build.sh's post-rootfs hooks
SCRIPT_DIR="$(dirname "$(realpath "$BASH_SOURCE")")"
"$SCRIPT_DIR/build.sh" post-rootfs $@
