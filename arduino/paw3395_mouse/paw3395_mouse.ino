/*
 * PAW3395 BLE Mouse Firmware
 * Target: Seeed XIAO nRF52840
 * Board package: Adafruit nRF52
 *   URL: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
 *   Board: "Adafruit Feather nRF52840 Express"
 *
 * Required libraries (install via Library Manager):
 *   - Adafruit NeoPixel  (SK6812MINI-E underglow LEDs)
 *
 * Built-in with Adafruit nRF52 board package:
 *   - bluefruit.h       (BLE stack)
 *   - BLEHidAdafruit.h  (BLE HID mouse/keyboard)
 *
 * Hardware connections (Seeed XIAO nRF52840 Dx labels):
 *   PAW3395 sensor (SPI):
 *     D6  (P1.11)  CS   – chip select, active-low
 *     D5  (P0.05)  IRQ  – motion interrupt, active-low
 *     D8  (P1.13)  SCK  – SPI clock
 *     D9  (P1.14)  MISO – SPI data in
 *     D10 (P1.15)  MOSI – SPI data out
 *   Buttons (active-low, internal pull-up):
 *     D1  (P0.03)  Function button (middle-click / BLE bond clear)
 *     D3  (P0.29)  Left mouse button
 *     D4  (P0.04)  Right mouse button
 *   EC11 scroll-wheel encoder:
 *     D2  (P0.28)  Encoder phase A
 *     D7  (P1.12)  Encoder phase B
 *   LEDs:
 *     D0  (P0.02)  SK6812MINI-E data line (2 LEDs, WS2812 protocol)
 *     P0.19        LED power enable – HIGH after 2-second boot delay
 *   Battery:
 *     A7  (P0.31)  Voltage-divider ADC input
 *     P0.14        Voltage-divider power (open-drain, active-low)
 *
 * Pin numbers below use the Adafruit nRF52 absolute-GPIO convention:
 *   Port 0 pins  → number = pin  (P0.N  = N)
 *   Port 1 pins  → number = 32 + pin  (P1.N  = 32 + N)
 */

#include <SPI.h>
#include <bluefruit.h>
#include <BLEHidAdafruit.h>
#include <Adafruit_NeoPixel.h>

// ---------------------------------------------------------------------------
// Pin definitions (Adafruit nRF52 absolute-GPIO numbering)
// ---------------------------------------------------------------------------
#define PIN_PAW_CS       (32 + 11)   // P1.11 – D6
#define PIN_PAW_IRQ      5            // P0.05 – D5
#define PIN_BTN_FUNC     3            // P0.03 – D1  (function / middle-click)
#define PIN_BTN_LEFT     29           // P0.29 – D3
#define PIN_BTN_RIGHT    4            // P0.04 – D4
#define PIN_ENC_A        28           // P0.28 – D2
#define PIN_ENC_B        (32 + 12)   // P1.12 – D7
#define PIN_LED_DATA     2            // P0.02 – D0
#define PIN_LED_POWER    19           // P0.19 – LED power enable
#define PIN_VBAT_CTRL    14           // P0.14 – battery ADC power (open-drain)
#define PIN_VBAT_ADC     A7           //         battery voltage input

// ---------------------------------------------------------------------------
// PAW3395 settings
// ---------------------------------------------------------------------------
#define PAW3395_CPI      1600         // default CPI (50–26000 in steps of 50)
#define PAW3395_PRODUCT_ID 0x51

// SPI speed: 2 MHz during init, 4 MHz during operation
#define SPI_FREQ_INIT    2000000
#define SPI_FREQ_RUN     4000000

// PAW3395 registers
#define REG_PRODUCT_ID      0x00
#define REG_REVISION_ID     0x01
#define REG_MOTION          0x02
#define REG_DELTA_X_L       0x03
#define REG_DELTA_X_H       0x04
#define REG_DELTA_Y_L       0x05
#define REG_DELTA_Y_H       0x06
#define REG_SQUAL           0x07
#define REG_RAWDATA_SUM     0x08
#define REG_MAX_RAWDATA     0x09
#define REG_MIN_RAWDATA     0x0A
#define REG_SHUTTER_LOWER   0x0B
#define REG_SHUTTER_UPPER   0x0C
#define REG_RESOLUTION_X_L  0x11
#define REG_RESOLUTION_X_H  0x12
#define REG_RESOLUTION_Y_L  0x13
#define REG_RESOLUTION_Y_H  0x14
#define REG_MOTION_BURST    0x16
#define REG_POWER_UP_RESET  0x3A
#define REG_SHUTDOWN        0x3B

// Write-bit mask for SPI
#define SPI_WRITE_BIT    0x80

// ---------------------------------------------------------------------------
// LED configuration
// ---------------------------------------------------------------------------
#define LED_COUNT        2
#define LED_BRIGHTNESS   80           // 0-255

// ---------------------------------------------------------------------------
// BLE HID mouse button bit-fields
// ---------------------------------------------------------------------------
#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
BLEHidAdafruit blehid;
BLEBas         blebas;
BLEDis         bledis;
Adafruit_NeoPixel leds(LED_COUNT, PIN_LED_DATA, NEO_GRBW + NEO_KHZ800);

// Motion data struct for burst read
struct MotionBurst {
  uint8_t motion;
  uint8_t observation;
  int16_t dx;
  int16_t dy;
  uint8_t squal;
  uint8_t rawDataSum;
  uint8_t maxRawData;
  uint8_t minRawData;
  uint16_t shutter;
};

// Button state (debounced) – tracks current HID button bitmask
uint8_t btnMask = 0;

// Encoder state
int8_t encLastA        = HIGH;
int32_t encAccum       = 0;           // accumulate ticks before sending scroll

// Timing
unsigned long lastBatteryMs = 0;
#define BATTERY_INTERVAL_MS 30000     // read battery every 30 s

// BLE connection flag
bool bleConnected = false;

// LED power delay: enable LEDs 2 s after boot
bool ledPowerEnabled   = false;
unsigned long bootMs   = 0;
#define LED_POWER_DELAY_MS 2000

// ---------------------------------------------------------------------------
// PAW3395 SPI helpers
// ---------------------------------------------------------------------------
static void pawSelect()   { digitalWrite(PIN_PAW_CS, LOW);  delayMicroseconds(1); }
static void pawDeselect() { delayMicroseconds(1); digitalWrite(PIN_PAW_CS, HIGH); delayMicroseconds(20); }

static void pawWrite(uint8_t addr, uint8_t data)
{
  SPI.beginTransaction(SPISettings(SPI_FREQ_INIT, MSBFIRST, SPI_MODE3));
  pawSelect();
  SPI.transfer(addr | SPI_WRITE_BIT);
  SPI.transfer(data);
  pawDeselect();
  SPI.endTransaction();
  delayMicroseconds(30);
}

static uint8_t pawRead(uint8_t addr)
{
  SPI.beginTransaction(SPISettings(SPI_FREQ_INIT, MSBFIRST, SPI_MODE3));
  pawSelect();
  SPI.transfer(addr & ~SPI_WRITE_BIT);
  delayMicroseconds(160);             // tSRAD
  uint8_t data = SPI.transfer(0x00);
  pawDeselect();
  SPI.endTransaction();
  delayMicroseconds(20);
  return data;
}

// ---------------------------------------------------------------------------
// PAW3395 performance-register init tables
// Two modes available: Gaming (low latency) and Office (power-saving).
// Values from PixArt PAW3395 application note / community-verified tables.
// ---------------------------------------------------------------------------

// Gaming performance mode register table { addr, value }
static const uint8_t kGamingMode[][2] = {
  {0x7F, 0x00}, {0x55, 0x01}, {0x50, 0x07}, {0x7F, 0x0E},
  {0x43, 0x10},
  {0x7F, 0x00}, {0x51, 0x7B}, {0x50, 0x00}, {0x55, 0x00},
  {0x7F, 0x00},
};

// ---------------------------------------------------------------------------
// PAW3395 initialisation
// ---------------------------------------------------------------------------
bool pawInit()
{
  // Hard reset via SPI: toggle CS
  pawDeselect();
  delay(50);

  // Perform power-up reset
  pawWrite(REG_POWER_UP_RESET, 0x5A);
  delay(5);

  // Read and discard initial motion registers (clears motion flag)
  (void)pawRead(REG_MOTION);
  (void)pawRead(REG_DELTA_X_L);
  (void)pawRead(REG_DELTA_X_H);
  (void)pawRead(REG_DELTA_Y_L);
  (void)pawRead(REG_DELTA_Y_H);

  // Verify product ID
  uint8_t pid = pawRead(REG_PRODUCT_ID);
  if (pid != PAW3395_PRODUCT_ID) {
    Serial.print("PAW3395: unexpected product ID 0x");
    Serial.println(pid, HEX);
    return false;
  }
  Serial.println("PAW3395: product ID OK (0x51)");

  // Apply gaming-mode performance registers
  for (size_t i = 0; i < sizeof(kGamingMode) / sizeof(kGamingMode[0]); i++) {
    uint8_t addr = kGamingMode[i][0];
    uint8_t val  = kGamingMode[i][1];
    pawWrite(addr, val);
  }

  // Set CPI
  pawSetCPI(PAW3395_CPI);

  // Increase motion SPI frequency now that init is done
  // (caller may reconstruct SPISettings at SPI_FREQ_RUN)
  return true;
}

// Set CPI (50–26000 in steps of 50)
void pawSetCPI(uint16_t cpi)
{
  cpi = constrain(cpi, 50, 26000);
  cpi = (cpi / 50) * 50;             // round to nearest 50

  // PAW3395 uses 16-bit resolution registers (X and Y independently)
  // Resolution = cpi * 25.4 / 25400 counts/mm  → register value = cpi/50 - 1
  uint16_t regVal = (cpi / 50) - 1;

  pawWrite(REG_RESOLUTION_X_L,  regVal & 0xFF);
  pawWrite(REG_RESOLUTION_X_H, (regVal >> 8) & 0xFF);
  pawWrite(REG_RESOLUTION_Y_L,  regVal & 0xFF);
  pawWrite(REG_RESOLUTION_Y_H, (regVal >> 8) & 0xFF);
}

// Read motion burst (12 bytes starting at 0x16)
bool pawReadBurst(MotionBurst *out)
{
  SPI.beginTransaction(SPISettings(SPI_FREQ_RUN, MSBFIRST, SPI_MODE3));
  pawSelect();

  SPI.transfer(REG_MOTION_BURST & ~SPI_WRITE_BIT);
  delayMicroseconds(35);             // tSRAD_MOTBR

  uint8_t buf[12];
  for (int i = 0; i < 12; i++) {
    buf[i] = SPI.transfer(0x00);
  }

  pawDeselect();
  SPI.endTransaction();

  out->motion     = buf[0];
  out->observation= buf[1];
  out->dx         = (int16_t)((buf[3] << 8) | buf[2]);
  out->dy         = (int16_t)((buf[5] << 8) | buf[4]);
  out->squal      = buf[6];
  out->rawDataSum = buf[7];
  out->maxRawData = buf[8];
  out->minRawData = buf[9];
  out->shutter    = (uint16_t)((buf[11] << 8) | buf[10]);

  return (out->motion & 0x80) != 0;  // bit 7 = MOT
}

// ---------------------------------------------------------------------------
// Battery level (percentage)
// ---------------------------------------------------------------------------
uint8_t readBatteryPercent()
{
  // Enable voltage-divider power (open-drain, active-low)
  digitalWrite(PIN_VBAT_CTRL, LOW);
  delayMicroseconds(100);

  int raw = analogRead(PIN_VBAT_ADC);

  // Disable (leave floating / high)
  digitalWrite(PIN_VBAT_CTRL, HIGH);

  // Voltage divider: 1M + 510k → Vbat = raw * (3.3V / 4095) * (1510k / 510k)
  float vbat = raw * (3.3f / 4095.0f) * (1510.0f / 510.0f);

  // Map 3.0V–4.2V to 0–100%
  uint8_t pct = (uint8_t)constrain((vbat - 3.0f) / (4.2f - 3.0f) * 100.0f, 0, 100);
  return pct;
}

// ---------------------------------------------------------------------------
// LED helpers
// ---------------------------------------------------------------------------
void setLedsOrange()
{
  // Color(R, G, B, W) – wire order handled by NEO_GRBW pixel type
  uint32_t col = leds.Color(255, 80, 0, 0);    // orange, no white
  for (int i = 0; i < LED_COUNT; i++) leds.setPixelColor(i, col);
  leds.show();
}

void setLedsOff()
{
  leds.clear();
  leds.show();
}

// ---------------------------------------------------------------------------
// BLE callbacks
// ---------------------------------------------------------------------------
void connectCallback(uint16_t conn_handle)
{
  (void)conn_handle;
  bleConnected = true;
  Serial.println("BLE: connected");
}

void disconnectCallback(uint16_t conn_handle, uint8_t reason)
{
  (void)conn_handle; (void)reason;
  bleConnected = false;
  Serial.println("BLE: disconnected – advertising");
  Bluefruit.Advertising.start(0);
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  bootMs = millis();

  Serial.begin(115200);
  // Give a moment for serial monitor to attach (optional, not blocking)
  delay(500);
  Serial.println("PAW3395 Mouse booting...");

  // ---- GPIO ----
  pinMode(PIN_PAW_CS,    OUTPUT);
  digitalWrite(PIN_PAW_CS, HIGH);

  pinMode(PIN_PAW_IRQ,   INPUT_PULLUP);
  pinMode(PIN_BTN_LEFT,  INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_FUNC,  INPUT_PULLUP);
  pinMode(PIN_ENC_A,     INPUT_PULLUP);
  pinMode(PIN_ENC_B,     INPUT_PULLUP);

  // LED power – keep off until boot delay expires
  pinMode(PIN_LED_POWER, OUTPUT);
  digitalWrite(PIN_LED_POWER, LOW);

  // Battery voltage-divider control (open-drain; HIGH = off)
  pinMode(PIN_VBAT_CTRL, OUTPUT);
  digitalWrite(PIN_VBAT_CTRL, HIGH);

  // nRF52840 ADC is 12-bit (0–4095)
  analogReadResolution(12);

  // ---- NeoPixel (keep off until LED_POWER enabled) ----
  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);
  setLedsOff();

  // ---- SPI ----
  SPI.begin();

  // ---- PAW3395 ----
  bool sensorOk = pawInit();
  if (!sensorOk) {
    Serial.println("WARNING: PAW3395 init failed – check wiring");
  }

  // ---- BLE ----
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Ribbon Mouse");
  Bluefruit.Periph.setConnectCallback(connectCallback);
  Bluefruit.Periph.setDisconnectCallback(disconnectCallback);

  blehid.begin();

  // BLE Device Information Service
  bledis.setManufacturer("PixArt / XIAO");
  bledis.setModel("PAW3395 Mouse");
  bledis.begin();

  // BLE Battery Service
  blebas.begin();
  blebas.write(readBatteryPercent());

  // Advertising
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.Advertising.addService(blebas);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.setInterval(32, 244); // fast / slow (units: 0.625 ms)
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  Serial.println("BLE advertising started");

  // Capture initial encoder state
  encLastA = digitalRead(PIN_ENC_A);
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  // --- LED power delay (enable LEDs 2 s after boot) ---
  if (!ledPowerEnabled && (millis() - bootMs >= LED_POWER_DELAY_MS)) {
    ledPowerEnabled = true;
    digitalWrite(PIN_LED_POWER, HIGH);
    setLedsOrange();
    Serial.println("LEDs enabled");
  }

  // --- PAW3395 motion ---
  // Poll via IRQ pin (active-low) or just read burst unconditionally
  if (digitalRead(PIN_PAW_IRQ) == LOW) {
    MotionBurst burst;
    if (pawReadBurst(&burst) && bleConnected) {
      // dx/dy: sensor +X = right, +Y = down (matches HID convention)
      // Clamp to int8 for HID report; large movements are split naturally
      // by polling loop iteration.
      int8_t rx = (int8_t)constrain(burst.dx, -127, 127);
      int8_t ry = (int8_t)constrain(burst.dy, -127, 127);
      if (rx != 0 || ry != 0) {
        blehid.mouseMove(rx, ry);
      }
    }
  }

  // --- Buttons ---
  // Build current bitmask from physical pin states
  uint8_t newMask = 0;
  if (digitalRead(PIN_BTN_LEFT)  == LOW) newMask |= MOUSE_BTN_LEFT;
  if (digitalRead(PIN_BTN_RIGHT) == LOW) newMask |= MOUSE_BTN_RIGHT;
  if (digitalRead(PIN_BTN_FUNC)  == LOW) newMask |= MOUSE_BTN_MIDDLE;

  if (newMask != btnMask) {
    btnMask = newMask;
    if (bleConnected) {
      if (btnMask) blehid.mouseButtonPress(btnMask);
      else          blehid.mouseButtonRelease();
    }
  }

  // --- Encoder (scrollwheel) ---
  int8_t encA = digitalRead(PIN_ENC_A);
  if (encA != encLastA) {
    int8_t encB = digitalRead(PIN_ENC_B);
    if (encB != encA) encAccum++;   // CW  → scroll down
    else              encAccum--;   // CCW → scroll up
    encLastA = encA;
  }
  // Send one tick at a time to avoid large scroll jumps
  if (encAccum != 0 && bleConnected) {
    int8_t tick = (encAccum > 0) ? 1 : -1;
    blehid.mouseScroll(tick);
    encAccum -= tick;
  }

  // --- Battery (periodic) ---
  unsigned long now = millis();
  if (now - lastBatteryMs >= BATTERY_INTERVAL_MS) {
    lastBatteryMs = now;
    uint8_t pct = readBatteryPercent();
    Serial.print("Battery: ");
    Serial.print(pct);
    Serial.println("%");
    blebas.write(pct);
  }

  // Small yield to prevent busy-looping the BLE stack
  delay(1);
}
