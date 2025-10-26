# ZERO

ZERO is a firmware (leveraging BLE COMPOSITE HID library) that allows ESP32 series boards to act as Xbox Series X/S controllers over Bluetooth Low Energy (BLE). It includes a web-based UI for quick calibration of joysticks and deadzone setup.

![ZERO Calibrator UI](https://github.com/manthanabc/ZERO/blob/main/images/ui.png?raw=true)

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration Mode](#configuration-mode)
- [Web Server & REST API](#web-server--rest-api)
- [Calibration Process](#calibration-process)
- [Technical Details](#technical-details)

## Features

- ✅ Xbox Series X/S controller emulation over BLE
- ✅ Full button and joystick support
- ✅ Dual rumble motor feedback
- ✅ Analog trigger support (LT/RT)
- ✅ D-Pad support
- ✅ Web-based calibration interface
- ✅ Persistent calibration storage (NVS)
- ✅ REST API for live monitoring and configuration
- ✅ Configurable deadzone settings

## Hardware Requirements

- **ESP32 Development Board** (ESP32-WROOM, ESP32-DevKitC, or compatible)
- **2x Analog Joysticks** (for left and right sticks)
- **16x Push Buttons** (for ABXY, D-Pad, triggers, etc.)
- **2x Rumble Motors** (optional, for haptic feedback)
- **Push-button switches** for analog triggers (LT/RT)

## Pin Configuration

### Analog Inputs (Joysticks)

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| Left Stick X | GPIO 32 | Left analog stick horizontal axis |
| Left Stick Y | GPIO 33 | Left analog stick vertical axis |
| Right Stick X | GPIO 35 | Right analog stick horizontal axis |
| Right Stick Y | GPIO 34 | Right analog stick vertical axis |

### Face Buttons

| Button | GPIO Pin | Description |
|--------|----------|-------------|
| A Button | GPIO 18 | Bottom face button |
| B Button | GPIO 4 | Right face button (RX2) |
| X Button | GPIO 15 | Left face button |
| Y Button | GPIO 16 | Top face button |

### D-Pad

| Direction | GPIO Pin | Description |
|-----------|----------|-------------|
| D-Pad Up | GPIO 12 | Directional pad up |
| D-Pad Down | GPIO 13 | Directional pad down |
| D-Pad Right | GPIO 27 | Directional pad right |
| D-Pad Left | GPIO 14 | Directional pad left |

### System Buttons

| Button | GPIO Pin | Description |
|--------|----------|-------------|
| Start | GPIO 19 | Start/Menu button |
| Select | GPIO 21 | Select/View button |

### Shoulder Buttons & Triggers

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| Left Bumper (LB) | GPIO 17 | Left shoulder button |
| Right Bumper (RB) | GPIO 5 | Right shoulder button |
| Left Trigger (LT) | GPIO 25 (TX2) | Left analog trigger switch |
| Right Trigger (RT) | GPIO 26 (D5) | Right analog trigger switch |

### Stick Buttons

| Button | GPIO Pin | Description |
|--------|----------|-------------|
| Left Stick Click (LS) | GPIO 3 | Press left analog stick |
| Right Stick Click (RS) | GPIO 1 | Press right analog stick |

### Rumble Motors

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| Strong Rumble | GPIO 22 | Large/strong rumble motor |
| Weak Rumble | GPIO 23 | Small/weak rumble motor |

### Wiring Notes

- All buttons use **INPUT_PULLUP** mode (active LOW)
- Connect buttons between GPIO pin and GND
- Analog sticks use the ESP32's built-in ADC (0-4095 range)
- Rumble motors are controlled via PWM (OUTPUT mode)

## Installation

### Prerequisites

1. **Arduino IDE** (version 1.8.x or 2.x) or **PlatformIO**
2. **ESP32 Board Support** installed in Arduino IDE
3. Required libraries:
   - `BleCompositeHID` (Xbox controller support)
   - `XboxGamepadDevice`
   - `NimBLEDevice`
   - `AsyncTCP`
   - `ESPAsyncWebServer`
   - `ArduinoJson`
   - `Preferences` (built-in)

### Steps

1. Clone or download this repository
2. Install the required libraries via Arduino Library Manager or PlatformIO
3. Open `config_zero.ino` in Arduino IDE
4. Select your ESP32 board from Tools → Board menu
5. Select the correct COM port
6. Click Upload

## Usage

### First-Time Setup

1. **Upload the firmware** to your ESP32 board
2. **Power on** the ESP32
3. The device will:
   - Start as a BLE Xbox controller (name: "ESP32 Controller")
   - Create a Wi-Fi Access Point (AP) named `ESP32-Cal`
   - Start a web server at `http://192.168.4.1`

### Connecting to a Host Device

1. On your PC/Console, enable Bluetooth
2. Search for new devices
3. Look for "ESP32 Controller" in the device list
4. Connect to it (no pairing code required)
5. The controller should be recognized as an Xbox Series X/S controller

### Basic Operation

- **BLE Connected**: When connected via Bluetooth, all buttons and sticks will send inputs to the host device
- **Disconnected**: When not connected, the device waits for a connection (no inputs are processed)
- **Rumble Feedback**: When a game sends rumble commands, the motors will vibrate accordingly

## Configuration Mode

The ESP32 continuously runs a **Wi-Fi Access Point** alongside the BLE controller functionality. This allows you to calibrate and configure the controller without interrupting gameplay.

### Accessing Configuration Mode

1. **Connect to the Wi-Fi AP**:
   - **SSID**: `ESP32-Cal`
   - **Password**: `esp32cal123`

2. **Open a web browser** and navigate to:
   ```
   http://192.168.4.1
   ```

3. **Open the Calibrator**:
   - Place the `configurator.html` file on your computer
   - Open it in a modern web browser (Chrome, Edge, Firefox)
   - The HTML file will connect to `http://192.168.4.1` automatically

### What Happens in Config Mode

- The **BLE controller remains active** - you can keep playing while calibrating
- The **web server** provides REST endpoints for:
  - Reading raw joystick values
  - Retrieving current settings
  - Updating calibration data
  - Adjusting deadzone percentages
- **No reboot required** - changes are saved to NVS (Non-Volatile Storage) and applied immediately

## Web Server & REST API

The firmware includes an **AsyncWebServer** running on port 80 that exposes the following REST endpoints:

### Endpoints

#### `GET /raw`

Returns raw ADC values from all four joystick axes (before calibration/deadzone).

**Response Example**:
```json
{
  "LX": 1960,
  "LY": 1927,
  "RX": 1975,
  "RY": 1940
}
```

**Use Case**: Live monitoring of joystick input for calibration

---

#### `GET /val`

Returns the processed (calibrated + deadzone applied) joystick values in Xbox stick format (-32767 to +32767).

**Response Example**:
```json
{
  "LX": 0,
  "LY": 125,
  "RX": -512,
  "RY": 0
}
```

**Use Case**: Testing calibrated output

---

#### `GET /settings`

Returns the current calibration settings and deadzone configuration.

**Response Example**:
```json
{
  "deadzone_percent": 5.0,
  "axes": {
    "LX": {
      "index": 0,
      "min": 470,
      "max": 3845,
      "center": 1960
    },
    "LY": {
      "index": 1,
      "min": 0,
      "max": 3180,
      "center": 1927
    },
    "RX": {
      "index": 2,
      "min": 302,
      "max": 3900,
      "center": 1975
    },
    "RY": {
      "index": 3,
      "min": 170,
      "max": 3538,
      "center": 1940
    }
  }
}
```

---

#### `POST /set`

Updates calibration settings and saves them to NVS.

**Request Body**:
```json
{
  "deadzone_percent": 5.0,
  "axes": {
    "LX": {
      "index": 0,
      "min": 470,
      "max": 3845,
      "center": 1960
    },
    "LY": {
      "index": 1,
      "min": 0,
      "max": 3180,
      "center": 1927
    }
  }
}
```

**Response**:
```json
{
  "ok": true
}
```

**Use Case**: Save calibration data from the web interface

---

### CORS Support

All endpoints support CORS (Cross-Origin Resource Sharing) to allow the standalone `configurator.html` to communicate with the ESP32 from any origin.

## Calibration Process

### Why Calibrate?

Analog joysticks have manufacturing variations that cause:
- Different min/max voltage ranges
- Off-center resting positions
- Drift and jitter

Calibration maps the raw ADC values to normalized stick values (-1.0 to +1.0) for accurate control.

### Using the Web Calibrator

1. **Connect to the ESP32-Cal Wi-Fi AP**

2. **Open `configurator.html`** in your browser

3. **Click "Start Calibration"**

4. **For each axis** (LX, LY, RX, RY):
   - Move the stick **slowly** through its full range
   - The interface records the **minimum** and **maximum** values
   - The **center** value is calculated when the stick is near neutral
   - Watch the live bar graph to ensure full movement is captured

5. **Adjust Deadzone**:
   - Set a percentage (0-50%) to eliminate stick drift
   - Typical values: 3-8%
   - Higher values = less sensitivity near center

6. **Click "Finish"** when all axes are calibrated

7. **Click "Save"** to write settings to the ESP32's flash memory

### Manual Calibration

You can also calibrate by:
1. Calling `GET /raw` while moving sticks
2. Recording min/max/center values manually
3. Posting them via `POST /set`

### Calibration Data Storage

- Settings are stored in **NVS (Non-Volatile Storage)**
- Namespace: `"cal"`
- Keys: `dz` (deadzone), `LX_min`, `LX_ctr`, `LX_max`, etc.
- Data persists across reboots

## Technical Details

### Deadzone Algorithm

The firmware applies a **scaled deadzone**:

```cpp
float applyDeadzone(float norm, float dzPercent) {
  float dz = dzPercent * 0.01f;
  float a = fabsf(norm);
  if (a <= dz) return 0.0f;
  float out = (a - dz) / (1.0f - dz);
  return (norm < 0) ? -out : out;
}
```

- Values within the deadzone → 0
- Values outside scale to maintain full range

### Normalization

Raw ADC values (0-4095) are normalized using three-point calibration:

```cpp
float normalizeFromCal_vals(int raw, int minV, int ctrV, int maxV) {
  if (raw >= ctrV) {
    return (float)(raw - ctrV) / (maxV - ctrV);  // 0 to +1
  } else {
    return -(float)(ctrV - raw) / (ctrV - minV); // -1 to 0
  }
}
```

### Button Logic

- All buttons use **active-LOW** logic with internal pull-ups
- Pressed = LOW (GND), Released = HIGH (3.3V)
- D-Pad uses **single-direction** mode (no diagonals in current implementation)

### BLE HID Profile

- Uses **XboxSeriesXControllerDeviceConfiguration**
- Advertises as a composite HID device
- Implements the Xbox gamepad report descriptor
- Supports vibration feedback via output reports

### Loop Timing

- When **connected**: 10ms delay (~100 Hz update rate)
- When **disconnected**: 20ms delay (reduces CPU usage)

## Troubleshooting

### Stick Drift

**Problem**: Sticks show input when not touched

**Solution**:
- Recalibrate and increase deadzone percentage
- Check for loose wiring on analog pins
- Ensure joystick potentiometers are in good condition

### Buttons Not Responding

**Problem**: Button presses don't register

**Solution**:
- Verify wiring (button between GPIO and GND)
- Check that buttons are active-LOW
- Test with a multimeter for continuity

### Can't Connect to BLE

**Problem**: Device not visible in Bluetooth list

**Solution**:
- Ensure ESP32 has sufficient power (5V 500mA+)
- Check Serial Monitor for "Starting composite HID device..."
- Restart the ESP32 and host device
- Make sure another device isn't already connected

### Web Server Not Accessible

**Problem**: Can't reach `http://192.168.4.1`

**Solution**:
- Verify you're connected to the `ESP32-Cal` Wi-Fi network
- Check Serial Monitor for the AP IP address
- Disable cellular data on mobile devices
- Try pinging `192.168.4.1` from terminal/cmd

### Calibration Not Saving

**Problem**: Settings revert after reboot

**Solution**:
- Check Serial Monitor for NVS errors
- Ensure `Preferences` library is installed
- Flash the ESP32 with "Erase Flash" option enabled once

## Advanced Configuration

### Changing Wi-Fi Credentials

Edit in `config_zero.ino`:

```cpp
const char* SSID = "ESP32-Cal";
const char* PASS = "esp32cal123";
```

### Adjusting Default Calibration

Edit in `config_zero.ino`:

```cpp
void loadDefaults() {
  settings.deadzone_percent = 5.0f;
  settings.LX = { 470, 1960, 3845, 0 };  // {min, center, max, index}
  settings.LY = {   0, 1927, 3180, 1 };
  settings.RX = { 302, 1975, 3900, 2 };
  settings.RY = { 170, 1940, 3538, 3 };
}
```

### Remapping Pins

To use different GPIO pins, modify the pin definitions:

```cpp
const int joyLX = 32;  // Change these to your preferred pins
const int joyLY = 33;
// ... etc
```

Then update your hardware wiring accordingly.

## License

This project uses the BLE Composite HID library for Xbox controller emulation. Please check individual library licenses for usage terms.

## Credits

- **BleCompositeHID**: Xbox Series X/S controller emulation
- **ESP-IDF**: ESP32 framework
- **AsyncWebServer**: Non-blocking web server
- **ArduinoJson**: JSON parsing and serialization

## Support

For issues, questions, or contributions, please open an issue on the project repository.

---

**Enjoy wireless Xbox gaming with your custom ESP32 controller!** 🎮
