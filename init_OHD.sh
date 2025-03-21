#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <username> <password>"
    exit 1
fi

# Assign command-line arguments to variables
SCIEBO_USERNAME="$1"
SCIEBO_PASSWORD="$2"

# File path in Sciebo
FILE_PATH="https://uni-wuppertal.sciebo.de/remote.php/webdav/openhd/X21/bigFiles.bz2"

# Download the file
echo "Downloading file from Sciebo..."
curl -u "$SCIEBO_USERNAME:$SCIEBO_PASSWORD" -O "$FILE_PATH"

# Check if the download was successful
if [ $? -eq 0 ]; then
    echo "Download complete."
else
    echo "Download failed. Please check your credentials and URL."
    exit 1
fi

# Extract the tar.gz file while preserving permissions
echo "Extracting the file..."
tar -xjvf bigFiles.tar.bz2 
ls -a
