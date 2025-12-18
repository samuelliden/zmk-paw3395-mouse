# PAW3395 Mouse Shield

This shield configures a Seeed Xiao nRF52840 with a PixArt PAW3395 optical sensor to act as a wireless mouse.

## Hardware Requirements

- Seeed Xiao nRF52840
- PixArt PAW3395 sensor connected via SPI
- Buttons for left, right, middle click (connected to GPIO pins)

## Pin Configuration

Adjust the pins in `paw3395.overlay` according to your wiring:

- SPI: MOSI (D8), MISO (D9), SCK (D10), CS (D7)
- Motion interrupt: D6
- Buttons: Left (D0), Right (D1), Middle (D2)

## Building

Use the build.yaml configuration to build the firmware.

## Usage

Flash the firmware to the Xiao nRF52840. It will appear as a Bluetooth mouse device.

Movement is handled by the sensor, buttons by the GPIO keys.