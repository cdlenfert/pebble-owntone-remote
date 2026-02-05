#!/bin/bash
# Install script for OwnTone Remote using Docker

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

if [ -z "$1" ]; then
    echo -e "${RED}Usage: ./docker-install.sh <phone-ip>${NC}"
    echo ""
    echo "Example: ./docker-install.sh 192.168.1.100"
    echo ""
    echo "To find your phone's IP:"
    echo "  Pebble app → Settings → Developer Connection"
    exit 1
fi

PHONE_IP=$1
IMAGE_NAME="rebble/pebble-sdk"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${GREEN}OwnTone Remote - Installing to Watch${NC}"
echo "====================================="
echo "Phone IP: $PHONE_IP"
echo ""

# Check if built
if [ ! -f "build/owntone-remote-new.pbw" ]; then
    echo -e "${YELLOW}App not built yet. Running build first...${NC}"
    ./docker-build.sh
    echo ""
fi

echo -e "${GREEN}Installing to watch...${NC}"
docker run --rm \
    -v "$PROJECT_DIR:/pebble" \
    -w /pebble \
    -e PEBBLE_PHONE=$PHONE_IP \
    $IMAGE_NAME \
    pebble install --phone $PHONE_IP

INSTALL_STATUS=$?

if [ $INSTALL_STATUS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ Installation successful!${NC}"
else
    echo ""
    echo -e "${RED}✗ Installation failed${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "1. Ensure phone and computer are on same WiFi"
    echo "2. Enable Developer Connection in Pebble app"
    echo "3. Verify phone IP address is correct"
    echo "4. Try restarting the Pebble app"
    exit 1
fi
