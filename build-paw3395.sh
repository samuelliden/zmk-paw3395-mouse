#!/bin/bash
# Build script for PAW3395 shield

# Set up environment
export CMAKE_PREFIX_PATH=/workspaces/unified-zmk-config-template/zephyr/share/zephyr-package/cmake

west build -s zmk/app -d build/paw3395_test -b xiao_ble -p -- \
  -DSHIELD=paw3395 \
  -DBOARD_ROOT=/workspaces/unified-zmk-config-template \
  -DZMK_CONFIG=/workspaces/unified-zmk-config-template/config

echo ""
echo "Build complete! Firmware location:"
echo "  build/paw3395_test/zephyr/zmk.uf2"
echo ""
echo "To flash:"
echo "  1. Double-click RESET on XIAO BLE to enter bootloader"
echo "  2. Copy zmk.uf2 to the bootloader drive"
