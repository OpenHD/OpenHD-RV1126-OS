#!/bin/sh

TARGET_GUID="614e0000-0000-4a59-8000-492b00004044"
DEVICE="/dev/mmcblk1"

# Check if any block device partition has the target GUID
if blkid | grep -q "$TARGET_GUID"; then
    echo "A partition with GUID $TARGET_GUID is already present. No action needed."
else
    echo "No partition with GUID $TARGET_GUID found. Running partition GUID modification..."
    sgdisk --partition-guid=6:$TARGET_GUID "$DEVICE" && \
    echo "Partition GUID modified successfully!"
    reboot
fi
