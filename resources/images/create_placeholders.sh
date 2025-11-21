#!/bin/bash
# Simple script to create placeholder images using sips (built into macOS)
# Creates 4 white placeholder images

for i in 0 1 2 3; do
  # Create a 56x72 white PNG image
  sips -s format png --setProperty formatOptions default -z 72 56 /System/Library/CoreServices/DefaultDesktop.heic --out "image_${i}.png" 2>/dev/null || \
  echo "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==" | base64 -d > "image_${i}.png"
done
echo "Placeholder images created (or use an image editor to create 56x72 PNG files)"
