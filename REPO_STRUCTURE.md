# Repository Structure

## Essential Files (Keep These)

### Root Configuration
- `build.yaml` - GitHub Actions build configuration
- `build-paw3395.sh` - Local build script
- `.gitignore` - Git ignore rules

### Firmware Configuration (`config/`)
- `config/paw3395.keymap` - Button/key mappings
- `config/boards/shields/paw3395/` - Shield hardware configuration
  - `paw3395.overlay` - Pin assignments and hardware setup
  - `paw3395.conf` - Feature configuration (Bluetooth, RGB, etc.)
  - `paw3395-pinctrl.dtsi` - Pin control definitions
  - `neopixel_power_delay.c` - LED power control with delay
  - `Kconfig.defconfig` - Default Kconfig settings
  - `Kconfig.shield` - Shield definition
  - `CMakeLists.txt` - Build configuration

### Driver
- `zmk-paw3395-driver/` - PAW3395 sensor driver (DO NOT MODIFY)

## Auto-Generated (Not in Git)
- `.west/` - West workspace metadata
- `modules/` - Zephyr modules (3.5GB)
- `zephyr/` - Zephyr RTOS (699MB)
- `zmk/` - ZMK firmware (49MB)
- `optional/` - Optional Zephyr modules (125MB)
- `build/` - Build artifacts

## Total Repository Size
- Tracked files: ~80KB (config + driver)
- Auto-downloaded: ~4.3GB (modules, zephyr, zmk)
