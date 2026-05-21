# MPO1 Orb — Capacitive Touch MIDI Controller

A Teensy 4.1 that reads a large metal orb as a capacitive touch sensor and sends MIDI CC messages (0–127) over USB. A potentiometer lets you tune sensitivity on the fly. A small OLED shows live readings.

---

## Parts

| Part | Details |
|---|---|
| Teensy 4.1 | Microcontroller — [pjrc.com/teensy41](https://www.pjrc.com/store/teensy41.html) |
| NFP1315-45A | 0.91" 128×32 OLED display (I2C) |
| 5k potentiometer | Linear taper |
| 10M ohm resistor | Mandatory for capacitive sensing |
| 100 ohm resistor | Optional — ESD protection |
| 4.7k ohm resistors × 2 | I2C pull-ups — skip if already on your OLED breakout (most are) |
| Wire | To connect Teensy to the metal orb |
| Metal orb | Must be electrically floating (not grounded anywhere) |

---

## Wiring

```
CAPACITIVE SENSOR
  Teensy Pin 2  ──[10M ohm]──  Teensy Pin 3 ──[100 ohm]──── wire to orb

POTENTIOMETER (5k)
  Teensy 3.3V ── left lug
  Teensy GND  ── right lug
  Teensy A0   ── center wiper

OLED DISPLAY (I2C)
  Teensy 3.3V ── VCC
  Teensy GND  ── GND
  Teensy 18   ── SDA
  Teensy 19   ── SCL
```

> **Important:** The orb must not touch ground. If it's grounded anywhere, the sensor will read maximum constantly.

---

## Software Setup

### 1. Install Arduino IDE + Teensyduino
- Download Arduino IDE 2.x from [arduino.cc](https://www.arduino.cc/en/software)
- Download and install Teensyduino from [pjrc.com/teensy/td_download.html](https://www.pjrc.com/teensy/td_download.html) — run the installer on top of Arduino IDE

### 2. Install libraries
Open Arduino IDE → **Sketch > Include Library > Manage Libraries**, then search and install:
- **CapacitiveSensor** by Paul Badger
- **Adafruit SSD1306** — when prompted, click **Install All** (pulls in Adafruit GFX and BusIO)

### 3. Configure the board
In Arduino IDE with the Teensy plugged in:
- **Tools > Board > Teensyduino > Teensy 4.1**
- **Tools > USB Type > MIDI** ← this is critical; the sketch won't compile without it
- **Tools > Port** → select the Teensy

### 4. Upload
Open `OrbMIDI/OrbMIDI.ino`, click **Upload**. Teensy Loader will flash the board automatically.

---

## OLED Display

```
CAP:  [raw capacitance reading]
MIDI: [current CC value 0–127]
MAX:  [sensitivity ceiling set by pot]
```

---

## Calibration (first time and weekly)

**1. Set the noise floor**
With no one near the orb, watch the **CAP** value. It should read near 0. If it's above 50, open the sketch and raise `CAP_THRESHOLD` to just above that number, then re-upload.

**2. Set the maximum**
Press a full palm flat on the orb and note the **CAP** reading. In the sketch, set `CAP_MAX_HIGH` to about 1.2× that value, then re-upload.

**3. Weekly tuning (no re-upload needed)**
Temperature and humidity shift capacitance. At the start of each session, let the orb sit powered for 10 minutes, then turn the pot to dial in the right feel.

---

## MIDI Output

- **CC number:** 1 (Mod Wheel) — change `MIDI_CC_NUM` in the sketch
- **Channel:** 1 — change `MIDI_CHANNEL` in the sketch
- Connects as a USB MIDI device; appears in any DAW or MIDI app automatically

---

## Tunable Constants

All settings are at the top of `OrbMIDI/OrbMIDI.ino`:

| Constant | Default | What it does |
|---|---|---|
| `CAP_THRESHOLD` | 50 | Raw values below this = no touch |
| `CAP_MAX_LOW` | 500 | Cap ceiling when pot is fully CCW |
| `CAP_MAX_HIGH` | 15000 | Cap ceiling when pot is fully CW |
| `CAP_SAMPLES` | 30 | Readings averaged per cycle (more = smoother, slower) |
| `EMA_ALPHA` | 0.2 | Smoothing factor (lower = smoother) |
| `MIDI_CC_NUM` | 1 | Which MIDI CC to send |
| `MIDI_CHANNEL` | 1 | MIDI channel (1–16) |

---

## Troubleshooting

**OLED stays blank** — check SDA/SCL wiring; try changing `OLED_ADDRESS` from `0x3C` to `0x3D` in the sketch

**CAP stays at 0** — confirm the 10M resistor is between pins 2 and 3, and the orb wire connects to pin 3 (not pin 2)

**CAP is pegged at max and doesn't respond** — orb is grounded somewhere; trace and remove the ground connection

**No MIDI device in DAW** — unplug/replug Teensy; confirm USB Type was set to MIDI before uploading

**Jittery readings** — increase `CAP_SAMPLES` to 50, or decrease `EMA_ALPHA` to 0.1
