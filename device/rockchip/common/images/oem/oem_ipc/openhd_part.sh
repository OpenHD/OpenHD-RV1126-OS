#!/bin/sh

FLAGFILE="/etc/.openhd_partition_done"
DEVICE="/dev/mmcblk1"

if [ ! -f "$FLAGFILE" ]; then
    echo "Running partition GUID modification..."
    sgdisk --partition-guid=6:614e0000-0000-4a59-8000-492b00004044 "$DEVICE" && \
    touch "$FLAGFILE" && \
    echo "Partition GUID modified successfully!"
    reboot
else
    echo "Partition GUID already modified. Skipping..."
fi
