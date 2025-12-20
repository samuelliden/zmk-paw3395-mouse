#!/bin/bash
# Build script for PAW3395 shield

west build -s zmk/app -d build/paw3395_test -b xiao_ble -- \
  -DSHIELD=paw3395 \
  -DBOARD_ROOT=/workspaces/unified-zmk-config-template/config

echo ""
echo "Build complete! Firmware location:"
echo "  build/paw3395_test/zephyr/zmk.uf2"
