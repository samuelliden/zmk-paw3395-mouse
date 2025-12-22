# Firmware Fix Instructions

## Problems Identified and Fixed

### 1. **CRITICAL: Missing kscan Configuration**
   - **Issue**: The main overlay file was missing `zmk,kscan = &kscan0;` in the `chosen` section
   - **Impact**: Without this, ZMK firmware won't initialize properly, preventing USB and Bluetooth from working
   - **Fix**: Added `zmk,kscan` to chosen section and configured kscan for buttons on D1, D3, D4

### 2. **Pin Configuration Errors**
   Based on your pin mapping spreadsheet:
   - **Encoder B-pin**: Changed from D3 to D7 (scrollwheel)
   - **Button Configuration**: Added kscan for D1 (button), D3 (mouse switch 1), D4 (mouse switch 2)
   - **Sensor pins**: D6 (CS), D5 (Motion) - these were already correct

### 3. **Missing Bluetooth Configuration**
   - **Issue**: Bluetooth was disabled in Kconfig.defconfig (`CONFIG_ZMK_BLE=n`)
   - **Fix**: Enabled Bluetooth support

### 4. **Kconfig Errors**
   - Removed invalid `WS2812_STRIP` config (should be set automatically)
   - Removed `EC11_TRIGGER_GLOBAL_THREAD` choice symbol default

## Pin Mapping Summary (Per Your Spreadsheet)
- **D0**: SK6812MINI-E Neopixels (2 LEDs)
- **D1**: B3U-1000P Button
- **D2**: Scrollwheel encoder A
- **D3**: Mouse switch 1 (Left click)
- **D4**: Mouse switch 2 (Right click)
- **D5**: PAW3395 MOTION signal
- **D6**: PAW3395 NCS (chip select)
- **D7**: Scrollwheel encoder B
- **D8**: PAW3395 SCLK
- **D9**: PAW3395 MISO
- **D10**: PAW3395 MOSI

## Flashing Instructions

### Step 1: Enter Bootloader Mode
1. Unplug the XIAO BLE from USB
2. Double-click the RESET button on the XIAO
3. The bootloader drive should appear as "XIAO-SENSE" or similar
4. The LED should pulse slowly (breathing effect)

### Step 2: Flash the New Firmware
1. Copy the firmware file to the bootloader drive:
   ```bash
   cp /workspaces/unified-zmk-config-template/build/paw3395_test/zephyr/zmk.uf2 /path/to/XIAO-SENSE/
   ```
   Or simply drag and drop the file from: `build/paw3395_test/zephyr/zmk.uf2`

2. The device will automatically reboot after the file is copied

### Step 3: Verify Functionality

**Expected Behavior:**
1. **Neopixels**: Should light up (may be dim initially, can be adjusted)
2. **USB**: Device should appear as a USB HID device
3. **Bluetooth**: Device should be discoverable as "Ribbon Mouse"
4. **Sensor**: Red LED on PAW3395 should be bright (not faint)
5. **Buttons**: 
   - D1: Sends F13 key (customize in keymap)
   - D3: Left mouse click
   - D4: Right mouse click
6. **Scrollwheel**: Should scroll up/down

**Testing:**
- Move the mouse → cursor should move
- Click D3 → left click
- Click D4 → right click
- Turn scrollwheel → page should scroll
- Press D1 → F13 key event

### Step 4: Pairing via Bluetooth (if needed)
1. On your computer, open Bluetooth settings
2. Look for "Ribbon Mouse"
3. Click to pair

## Troubleshooting

### If device still doesn't appear:
1. **Check bootloader**: Make sure you're entering bootloader (double-click RESET, not single-click)
2. **Verify file copy**: The file should be exactly 421KB
3. **Try different USB cable**: Some cables are charge-only
4. **Check USB port**: Try a different port
5. **Serial output**: Connect via serial at 115200 baud to see boot messages

### If neopixels are still dim:
The firmware includes a power delay for the neopixels. If they're too dim, you can adjust the configuration or check the power connections.

### If sensor is still faint:
The sensor should have a bright red LED when working. If it's faint, there may be a hardware connection issue with power (3V3) or the sensor initialization might be failing.

## Rebuild Firmware (if needed)

To rebuild with changes:
```bash
cd /workspaces/unified-zmk-config-template
export CMAKE_PREFIX_PATH=/workspaces/unified-zmk-config-template/zephyr/share/zephyr-package/cmake
west build -s zmk/app -d build/paw3395_test -b xiao_ble -p -- \
  -DSHIELD=paw3395 \
  -DBOARD_ROOT=/workspaces/unified-zmk-config-template \
  -DZMK_CONFIG=/workspaces/unified-zmk-config-template/config
```

The firmware will be at: `build/paw3395_test/zephyr/zmk.uf2`

## Files Modified
1. `boards/shields/paw3395/paw3395.overlay` - Added kscan, fixed encoder pin
2. `boards/shields/paw3395/paw3395.conf` - Added Bluetooth config
3. `boards/shields/paw3395/Kconfig.defconfig` - Fixed Kconfig errors, enabled BT
4. `config/paw3395.keymap` - Updated keymap with mouse buttons and scrollwheel

## What Was Wrong Before

The previous firmware appeared to initialize the hardware (sensor worked, neopixels tried to work) but ZMK itself wasn't properly starting because it couldn't find a valid keyboard matrix (`kscan`). This is a required component for ZMK - even for a mouse, it needs at least one button defined. Without it:
- USB HID won't enumerate
- Bluetooth won't advertise
- The device boots but doesn't register as an input device

Now with the kscan properly configured, ZMK should fully initialize and register as both a keyboard and mouse HID device.
