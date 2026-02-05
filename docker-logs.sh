#!/bin/bash
# View logs from the Pebble app using Docker

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

if [ -z "$1" ]; then
    echo -e "${RED}Usage: ./docker-logs.sh <phone-ip>${NC}"
    echo ""
    echo "Example: ./docker-logs.sh 192.168.1.100"
    exit 1
fi

PHONE_IP=$1
IMAGE_NAME="rebble/pebble-sdk"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${GREEN}OwnTone Remote - Live Logs${NC}"
echo "=========================="
echo "Phone IP: $PHONE_IP"
echo "Press Ctrl+C to exit"
echo ""

docker run --rm -it \
    -v "$PROJECT_DIR:/pebble" \
    -w /pebble \
    -e PEBBLE_PHONE=$PHONE_IP \
    $IMAGE_NAME \
    pebble logs --phone $PHONE_IP
