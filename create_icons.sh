#!/bin/bash

# Script to create simple placeholder icons for OwnTone Remote
# Requires ImageMagick (install with: brew install imagemagick)

ICON_DIR="resources/images"
SIZE=25

mkdir -p "$ICON_DIR"

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "ImageMagick is required but not installed."
    echo "Install it with: brew install imagemagick"
    echo ""
    echo "Alternatively, you can:"
    echo "1. Find free 25x25 icons online"
    echo "2. Copy icons from another Pebble project"
    echo "3. Create your own in an image editor"
    exit 1
fi

echo "Creating placeholder icons..."

# Use magick instead of deprecated convert
if command -v magick &> /dev/null; then
    MAGICK_CMD="magick"
else
    MAGICK_CMD="convert"
fi

# Play icon (right-pointing triangle)
$MAGICK_CMD -size ${SIZE}x${SIZE} xc:none \
    -fill white -draw "polygon 5,5 5,20 20,12.5" \
    "$ICON_DIR/icon_play.png"

# Pause icon (two vertical bars)
$MAGICK_CMD -size ${SIZE}x${SIZE} xc:none \
    -fill white -draw "rectangle 7,5 11,20 rectangle 14,5 18,20" \
    "$ICON_DIR/icon_pause.png"

# Next icon (skip forward: triangle + bar)
$MAGICK_CMD -size ${SIZE}x${SIZE} xc:none \
    -fill white -stroke white -strokewidth 1.5 \
    -draw "polygon 4,4 4,21 15,12.5 line 18,4 18,21" \
    "$ICON_DIR/icon_next.png"

# Previous icon (skip backward: bar + triangle)
$MAGICK_CMD -size ${SIZE}x${SIZE} xc:none \
    -fill white -stroke white -strokewidth 1.5 \
    -draw "line 7,4 7,21 polygon 10,12.5 21,4 21,21" \
    "$ICON_DIR/icon_prev.png"

echo "Icons created in $ICON_DIR/"
echo "You can now run: pebble build"
