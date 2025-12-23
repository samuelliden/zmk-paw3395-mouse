#!/bin/bash
# Build script for PAW3395 shield

# Get build number from environment or use timestamp
BUILD_NUM="${BUILD_NUMBER:-$(date +%Y%m%d-%H%M%S)}"

# Set up environment
export CMAKE_PREFIX_PATH=/workspaces/unified-zmk-config-template/zephyr/share/zephyr-package/cmake

west build -s zmk/app -d build/paw3395_test -b xiao_ble -p -- \
  -DSHIELD=paw3395 \
  -DBOARD_ROOT=/workspaces/unified-zmk-config-template \
  -DZMK_CONFIG=/workspaces/unified-zmk-config-template/config

# Create versioned output
OUTPUT_DIR="build/paw3395_test/zephyr"
FIRMWARE_NAME="paw3395-xiao_ble-zmk-${BUILD_NUM}.uf2"

if [ -f "${OUTPUT_DIR}/zmk.uf2" ]; then
    cp "${OUTPUT_DIR}/zmk.uf2" "${OUTPUT_DIR}/${FIRMWARE_NAME}"
    echo ""
    echo "Build complete! Firmware locations:"
    echo "  ${OUTPUT_DIR}/zmk.uf2"
    echo "  ${OUTPUT_DIR}/${FIRMWARE_NAME}"
else
    echo ""
    echo "Build complete! Firmware location:"
    echo "  ${OUTPUT_DIR}/zmk.uf2"
fi

echo ""
echo "To flash:"
echo "  1. Double-click RESET on XIAO BLE to enter bootloader"
echo "  2. Copy zmk.uf2 to the bootloader drive"
