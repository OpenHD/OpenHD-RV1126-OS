#!/bin/sh

DEVICE="/dev/mmcblk1"
PARTITION_NUM=6
TARGET_GUID="614e0000-0000-4a59-8000-492b00004044"
TRIGGER_FILE="/etc/init.d/S40_recovery"

if [ -f "$TRIGGER_FILE" ]; then
    echo "$TRIGGER_FILE found. Running partition GUID modification..."
    sgdisk --partition-guid=$PARTITION_NUM:$TARGET_GUID "$DEVICE" && \
    echo "Partition GUID modified successfully!"
    reboot
else
    echo "$TRIGGER_FILE not found. No action taken."
fi
