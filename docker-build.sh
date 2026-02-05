#!/bin/bash
# Build script for OwnTone Remote using Docker

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}OwnTone Remote - Docker Build${NC}"
echo "================================"
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo -e "${YELLOW}Docker is not running. Please start Docker Desktop.${NC}"
    exit 1
fi

# Use Rebble SDK image
IMAGE_NAME="rebble/pebble-sdk"
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo -e "${YELLOW}Pulling Rebble Pebble SDK image...${NC}"
    docker pull $IMAGE_NAME
    echo ""
fi

# Get the project directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the build
echo -e "${GREEN}Building OwnTone Remote...${NC}"
docker run --rm \
    -v "$PROJECT_DIR:/pebble" \
    -w /pebble \
    $IMAGE_NAME \
    pebble build

BUILD_STATUS=$?

if [ $BUILD_STATUS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo ""
    echo "Output: build/owntone-remote-new.pbw"
    echo ""
    echo "To install on your watch:"
    echo "1. Ensure your phone is on the same WiFi network"
    echo "2. Enable Developer Connection in Pebble app"
    echo "3. Run: ./docker-install.sh <phone-ip>"
else
    echo ""
    echo -e "${YELLOW}✗ Build failed${NC}"
    exit 1
fi
