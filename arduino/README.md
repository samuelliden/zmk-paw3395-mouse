# PAW3395 Mouse – Arduino IDE Firmware

Arduino IDE sketch for the **Seeed XIAO nRF52840 + PAW3395** mouse hardware.  
This is an alternative to the ZMK firmware that lets you flash and test the hardware with a familiar Arduino workflow.

## Hardware

| Part | Notes |
|------|-------|
| Seeed XIAO nRF52840 | main MCU |
| PixArt PAW3395 | optical sensor via SPI |
| SK6812MINI-E × 2 | RGBW underglow LEDs |
| B3U-1000P tactile | function / middle-click button |
| Mouse switch × 2 | left / right click |
| EC11 rotary encoder | scroll wheel |

### Wiring

| Signal | XIAO pin | nRF52840 GPIO |
|--------|----------|---------------|
| PAW3395 CS | D6 | P1.11 |
| PAW3395 IRQ | D5 | P0.05 |
| SPI SCK | D8 | P1.13 |
| SPI MISO | D9 | P1.14 |
| SPI MOSI | D10 | P1.15 |
| Left button | D3 | P0.29 |
| Right button | D4 | P0.04 |
| Function button | D1 | P0.03 |
| Encoder A | D2 | P0.28 |
| Encoder B | D7 | P1.12 |
| LED data | D0 | P0.02 |
| LED power | – | P0.19 |
| Battery ADC | A7 | P0.31 |
| Battery ADC power | – | P0.14 |

## Setup

### 1 – Install the Adafruit nRF52 board package

1. Open **Arduino IDE → Preferences**.
2. Add the following URL to *Additional Boards Manager URLs*:
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
3. Open **Tools → Board → Boards Manager**, search for **Adafruit nRF52** and install it.
4. Select **Tools → Board → Adafruit nRF52840 Feather Express**.

> **Seeed board package alternative:** If you prefer the Seeed nRF52840 package, the sketch compiles with minor pin-number adjustments.  The Adafruit package is recommended because it includes the Bluefruit BLE HID library.

### 2 – Install required libraries

Open **Tools → Manage Libraries** and install:

| Library | Version tested |
|---------|---------------|
| **Adafruit NeoPixel** | ≥ 1.11 |

The following are bundled with the Adafruit nRF52 board package and do **not** need to be installed separately:
- `bluefruit.h`
- `BLEHidAdafruit.h`

### 3 – Open and upload the sketch

1. Open `arduino/paw3395_mouse/paw3395_mouse.ino` in Arduino IDE.
2. Connect the XIAO nRF52840 via USB.
3. Select the correct **Port**.
4. Click **Upload**.

## Usage

| Action | Result |
|--------|--------|
| Move sensor | Bluetooth mouse cursor |
| Press D3 | Left click |
| Press D4 | Right click |
| Press D1 | Middle click |
| Turn encoder | Scroll wheel |
| Boot | Orange LEDs appear 2 s after power-on |

### Bluetooth pairing

The device advertises as **"Ribbon Mouse"**.  Pair it from your OS Bluetooth settings.  
On reconnect, the bond is restored automatically.

### Serial monitor

Open the serial monitor at **115 200 baud** to see:
- PAW3395 product-ID confirmation on boot
- Battery percentage (every 30 s)
- BLE connect/disconnect events

## Customisation

| Constant | Location | Default | Description |
|----------|----------|---------|-------------|
| `PAW3395_CPI` | top of `.ino` | `1600` | Sensor CPI (50–26000, multiples of 50) |
| `LED_BRIGHTNESS` | top of `.ino` | `80` | LED brightness (0–255) |
| `LED_POWER_DELAY_MS` | top of `.ino` | `2000` | ms before LEDs turn on |
| `BATTERY_INTERVAL_MS` | top of `.ino` | `30000` | Battery-read interval (ms) |

## Notes

- **P0.19 (LED power)** is the SWCLK debug pin.  It operates fine as a GPIO; however, SWD debugging is unavailable while it is in use.
- **CPI range:** The PAW3395 supports 50–26000 CPI in steps of 50.  The ZMK config uses 26000 CPI; 1600 CPI is a comfortable desktop default for Arduino testing.
- Large mouse movements (> 127 counts per poll) are clamped to ±127 per HID report.  Increase the polling rate or use multiple reports per loop iteration if needed at very high CPI.
