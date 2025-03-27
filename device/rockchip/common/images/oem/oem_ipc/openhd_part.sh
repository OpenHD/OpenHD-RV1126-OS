#!/bin/sh

DEVICE="/dev/mmcblk1"
PARTITION_NUM=6
TARGET_GUID="614e0000-0000-4a59-8000-492b00004044"

CURRENT_GUID=$(sgdisk -i $PARTITION_NUM "$DEVICE" | grep "Partition unique GUID" | awk '{print $4}')

if [ "$CURRENT_GUID" = "$TARGET_GUID" ]; then
    echo "Partition $PARTITION_NUM already has GUID $TARGET_GUID. No action needed."
else
    echo "Partition $PARTITION_NUM has GUID $CURRENT_GUID. Updating to $TARGET_GUID..."
    sgdisk --partition-guid=$PARTITION_NUM:$TARGET_GUID "$DEVICE" && \
    echo "Partition GUID updated successfully!"
    reboot
fi
