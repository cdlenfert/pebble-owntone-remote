#!/bin/bash
# Open an interactive shell in the Pebble SDK container

IMAGE_NAME="rebble/pebble-sdk"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Opening Pebble SDK shell..."
echo "You can run any pebble commands here."
echo ""

docker run --rm -it \
    -v "$PROJECT_DIR:/pebble" \
    -w /pebble \
    $IMAGE_NAME \
    /bin/bash
