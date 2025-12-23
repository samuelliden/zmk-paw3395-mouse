# PAW3395 Mouse Shield

This shield configures a Seeed Xiao nRF52840 with a PixArt PAW3395 optical sensor to act as a wireless mouse.

## Hardware Requirements

- Seeed Xiao nRF52840
- PixArt PAW3395 sensor connected via SPI
- SK6812MINI-E RGB LEDs (2x)
- B3U-1000P tactile button
- EC11 rotary encoder (scrollwheel)
- Mouse switches (2x)

## Pin Configuration

Current pin assignments (configured in `paw3395.overlay`):

**PAW3395 Sensor (SPI):**
- D6 (P1.11) - CS/NCS (Chip Select)
- D5 (P0.05) - Motion/IRQ (Interrupt)
- D8 (P1.13) - SCK (SPI Clock)
- D9 (P1.14) - MISO (SPI Data In)
- D10 (P1.15) - MOSI (SPI Data Out)

**Input Devices:**
- D1 (P0.03) - B3U-1000P Button (sends "F" key, hold for Bluetooth controls)
- D3 (P0.29) - Mouse Switch 1 (Left Click)
- D4 (P0.04) - Mouse Switch 2 (Right Click)
- D2 (P0.28) + D7 (P1.12) - EC11 Encoder (Scrollwheel)

**LEDs:**
- D0 (P0.02) - SK6812MINI-E RGB LEDs (2x, WS2812 SPI protocol)
- AA24 (P0.19/SWCLK) - LED Power Control (enabled with 2s delay)

**Note:** P0.19 (SWCLK) is reconfigured as GPIO via `CONFIG_GPIO_AS_PINRESET=y`, disabling SWD debugging.

## Building

### Local Build
```bash
./build-paw3395.sh
```

### GitHub Actions
Push to main branch. Firmware artifacts will be versioned as `paw3395-xiao_ble-zmk-v{run_number}.uf2`

## Flashing

1. Double-click RESET button on XIAO BLE to enter bootloader mode
2. Copy `zmk.uf2` to the bootloader drive that appears
3. Device will reboot automatically

## Usage

- **USB:** Plug in and works immediately as keyboard + mouse
- **Bluetooth:** 
  - Device name: "Ribbon Mouse"
  - To clear bonds: Hold D1 button for 3 seconds on boot
  - LEDs turn on 2 seconds after boot

## Features

- High-precision PAW3395 sensor (26000 CPI)
- 2x RGB underglow LEDs (orange by default)
- 3-button mouse (left, right, center/button)
- Scrollwheel
- Battery monitoring
- Bluetooth LE wireless