#!/bin/bash
cd "$(dirname "$0")/images"

# Volume Up icon (triangle pointing up)
convert -size 25x25 xc:transparent \
  -fill white -draw "polygon 4,18 12.5,7 21,18" \
  icon_volume_up.png

# Volume Down icon (triangle pointing down)
convert -size 25x25 xc:transparent \
  -fill white -draw "polygon 4,7 12.5,18 21,7" \
  icon_volume_down.png

echo "Volume icons created"
ls -la icon_volume*.png
