# Docker Build Guide

Complete guide for building OwnTone Remote using Docker (no local Pebble SDK required!)

## Quick Start

### 1. Start Docker Desktop
Make sure Docker Desktop is running on your Mac.

### 2. Build the App
```bash
cd owntone-remote-new
./docker-build.sh
```

This will:
- Build a Docker image with Pebble SDK (first time only, ~5-10 min)
- Compile your app
- Output: `build/owntone-remote-new.pbw`

### 3. Install to Watch
```bash
./docker-install.sh <your-phone-ip>
```

Example:
```bash
./docker-install.sh 192.168.1.100
```

**How to find your phone IP:**
- Open Pebble app on phone
- Go to Settings → Developer Connection
- Note the IP address shown

## All Docker Commands

### Build Only
```bash
./docker-build.sh
```
Compiles the app without installing.

### Build + Install
```bash
./docker-build.sh && ./docker-install.sh 192.168.1.100
```
One-liner to build and install.

### View Logs
```bash
./docker-logs.sh 192.168.1.100
```
Stream live logs from your watch (useful for debugging).

### Interactive Shell
```bash
./docker-shell.sh
```
Opens a shell in the Pebble SDK container. You can run any `pebble` commands:
```bash
pebble build
pebble clean
pebble install --phone 192.168.1.100
pebble logs --phone 192.168.1.100
```

## Troubleshooting

### Docker Not Running
```
Error: Cannot connect to Docker daemon
```
**Solution:** Start Docker Desktop

### Build Fails
```bash
# Clean and rebuild
docker rmi pebble-sdk  # Remove old image
./docker-build.sh       # Rebuild from scratch
```

### Install Fails

**Problem: Phone not found**
- Ensure phone and Mac are on same WiFi
- Enable Developer Connection in Pebble app
- Verify IP address is correct
- Try restarting Pebble app

**Problem: Connection timeout**
- Check firewall settings
- Make sure no VPN is active
- Try from Pebble app: Settings → Developer Connection → Reconnect

### Icons Missing
If you see resource errors during build:
```bash
# Recreate icons
./create_icons.sh

# Then rebuild
./docker-build.sh
```

## Docker Image Details

**Base:** Ubuntu 18.04  
**Python:** 2.7 (required by Pebble SDK)  
**Pebble SDK:** v4.5  
**Size:** ~1.5 GB (first build)

The image is cached after first build, so subsequent builds are fast (<10 seconds).

## Manual Docker Commands

If you prefer to run Docker manually:

```bash
# Build image
docker build -t pebble-sdk .

# Build app
docker run --rm \
  -v "$(pwd):/pebble-project" \
  -w /pebble-project \
  pebble-sdk \
  pebble build

# Install app
docker run --rm \
  -v "$(pwd):/pebble-project" \
  -w /pebble-project \
  pebble-sdk \
  pebble install --phone 192.168.1.100
```

## Comparison: Docker vs Local SDK

| Feature | Docker | Local SDK |
|---------|--------|-----------|
| Setup time | ~10 min (first time) | ~30-60 min |
| Disk space | ~1.5 GB | ~2-3 GB |
| Dependencies | Just Docker | Python 2.7, virtualenv, etc |
| Isolation | ✓ Clean environment | Uses system Python |
| Portability | ✓ Works anywhere | Platform-specific |
| Updates | Rebuild image | Manual updates |

## Next Steps

After successful build and install:

1. **Test the app** on your watch
2. **View logs** to debug: `./docker-logs.sh <phone-ip>`
3. **Make changes** to code
4. **Rebuild**: `./docker-build.sh`
5. **Reinstall**: `./docker-install.sh <phone-ip>`

## Development Workflow

```bash
# Edit code in your favorite editor
vim src/windows/player.c

# Build and test
./docker-build.sh
./docker-install.sh 192.168.1.100

# Watch logs while testing
./docker-logs.sh 192.168.1.100

# Repeat!
```

## Clean Everything

```bash
# Remove build artifacts
rm -rf build/

# Remove Docker image (to rebuild from scratch)
docker rmi pebble-sdk

# Remove all stopped containers
docker container prune
```
