#!/bin/sh

DEVICE="/dev/mmcblk1"
ROOT_MOUNT_SOURCE=$(findmnt -n -o SOURCE /)

if [ "$ROOT_MOUNT_SOURCE" != "/dev/root" ]; then
    echo "Running partition GUID modification..."
    sgdisk --partition-guid=6:614e0000-0000-4a59-8000-492b00004044 "$DEVICE" && \
    echo "Partition GUID modified successfully!"
    reboot
else
    echo "/ is already mounted from /dev/root. No action needed."
fi
