#!/bin/bash

input_video=$1

if [ -z "$input_video" ]; then
    echo "Please provide a video file as input"
    echo "Usage: $0 <video_file>"
    exit 1
fi
# Analyze the video properties 
echo "Video Analysis:"
ffmpeg -i "$input_video" -f null - 2>&1

# Look at the first 128 bytes of binary data
echo -e "\nBinary Analysis (first 128 bytes):"
xxd -b -l 128 "$input_video"
