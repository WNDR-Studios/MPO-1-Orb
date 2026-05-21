# MPO1 Orb — Teensy 4.1 Capacitive MIDI Controller

## Project Overview
Capacitive touch sensor for a large metal orb. Reads proximity/touch intensity, maps it to a MIDI CC value (0–127), and outputs over USB MIDI. A potentiometer adjusts the sensitivity ceiling in real-time so the instrument can be tuned for temperature and humidity changes between installations. A 128×32 OLED shows live readings.

## Hardware

| Component | Part | Notes |
|---|---|---|
| Microcontroller | Teensy 4.1 | USB MIDI mode required |
| Display | NFP1315-45A | SSD1306 controller, 128×32, I2C at 0x3C |
| Potentiometer | 5k linear | Wiper to A0 |
| Resistor (cap sense) | 10M ohm | Between send and receive pins — mandatory |
| Resistor (ESD) | 100 ohm | Series on receive pin before wire to orb — optional but recommended |
| I2C pull-ups | 4.7k ohm × 2 | SDA and SCL to 3.3V; check if already on OLED breakout |

## Wiring

```
Capacitive Sensor:
  Teensy Pin 2  ──[10M ohm]──  Teensy Pin 3 ──[100 ohm]── wire to metal orb

  The orb must be electrically FLOATING (not connected to GND or any rail).
  Grounding the orb will max out the reading permanently.

Potentiometer (5k):
  3.3V  ── left lug
  GND   ── right lug
  A0    ── wiper (center lug)

  Use 3.3V — NOT Vin/5V — to stay within the Teensy 4.1 ADC input range.

OLED (NFP1315-45A, I2C):
  3.3V  ── VCC
  GND   ── GND
  Pin 18 (SDA) ── SDA
  Pin 19 (SCL) ── SCL
```

## Pin Summary

| Teensy Pin | Function |
|---|---|
| 2 | Cap sensor SEND |
| 3 | Cap sensor RECEIVE → 100 ohm → orb |
| A0 (14) | Pot wiper |
| 18 | OLED SDA |
| 19 | OLED SCL |
| 3.3V | Pot high rail, OLED VCC |
| GND | Pot low rail, OLED GND |

## Libraries (Arduino Library Manager)

1. **CapacitiveSensor** by Paul Badger / Paul Stoffregen
2. **Adafruit SSD1306** — when prompted, click "Install All" (pulls in Adafruit GFX + BusIO)
3. Wire, usbMIDI — built into Teensyduino, no install needed

## Arduino IDE Setup

1. Install Teensyduino from pjrc.com if not already done
2. Board: `Teensy 4.1`
3. **Tools > USB Type > MIDI** ← required; without this, `usbMIDI` won't compile
4. Verify (checkmark) before uploading to confirm all libraries resolve
5. Upload — Teensy Loader flashes automatically

After switching USB Type to MIDI for the first time, the serial COM port disappears and reappears as a MIDI device. This is expected.

## Sketch File

`OrbMIDI/OrbMIDI.ino`

All tunable values are constants at the top of the file:

| Constant | Default | Purpose |
|---|---|---|
| `CAP_SEND_PIN` | 2 | CapacitiveSensor send pin |
| `CAP_RECEIVE_PIN` | 3 | CapacitiveSensor receive pin |
| `POT_PIN` | A0 | Potentiometer wiper pin |
| `MIDI_CC_NUM` | 1 | MIDI CC number (1 = Mod Wheel) |
| `MIDI_CHANNEL` | 1 | MIDI channel 1–16 |
| `CAP_SAMPLES` | 30 | Samples per reading; raise for stability |
| `CAP_THRESHOLD` | 50 | Below this raw value = no touch |
| `CAP_MAX_LOW` | 500 | Cap ceiling when pot is fully CCW |
| `CAP_MAX_HIGH` | 15000 | Cap ceiling when pot is fully CW |
| `EMA_ALPHA` | 0.2 | Smoothing factor (lower = smoother) |
| `LOOP_INTERVAL_MS` | 20 | Update rate (~50 Hz) |
| `OLED_ADDRESS` | 0x3C | Try 0x3D if display fails to init |

## OLED Display Layout

```
CAP:  [raw smoothed reading]
MIDI: [0–127 value]
MAX:  [current ceiling from pot]
```

## Calibration Workflow

Run this after first upload and at the start of each installation week.

### 1. Check noise floor
With no one near the orb, observe the **CAP** value on the OLED. It should sit below `CAP_THRESHOLD` (default 50). If it's above 50, increase `CAP_THRESHOLD` to `noise_floor + 20` and re-upload.

### 2. Find your max touch value
Place a full palm flat on the orb. Note the **CAP** value. Set `CAP_MAX_HIGH` to approximately 1.2× that value and re-upload. This ensures the pot's full CW position maps to a firm-palm contact.

### 3. Set the minimum sensitivity
`CAP_MAX_LOW` controls how sensitive the instrument is when the pot is fully CCW. A lower value makes it respond to very light contact or even proximity. Start at 500 and adjust by feel.

### 4. Weekly in-situ tuning (no re-upload needed)
Let the orb sit powered for 10 minutes to thermally stabilize, then use the pot to dial sensitivity for the current environment. The **MAX** value on the OLED shows the live ceiling.

### Typical starting ranges by orb size

| Orb diameter | CAP_MAX_LOW | CAP_MAX_HIGH |
|---|---|---|
| ~4 in (softball) | 200 | 5000 |
| ~9 in (basketball) | 500 | 15000 |
| ~18 in+ | 1000 | 30000 |

## Verification Checklist

- [ ] OLED: splash screen "ORB MIDI CTRL" appears for 1.5s, then three live data lines
- [ ] If OLED stays blank: check VCC/GND/SDA/SCL; try `OLED_ADDRESS = 0x3D`
- [ ] CAP value rises when hand approaches orb; drops when hand moves away
- [ ] Pot full sweep: MAX value on OLED moves from `CAP_MAX_LOW` to `CAP_MAX_HIGH`
- [ ] MIDI: open MIDI-OX (Windows) or a DAW and confirm CC 1 values 0–127 arrive on channel 1
- [ ] No spurious MIDI messages sent when orb is untouched (adjust `CAP_THRESHOLD` if needed)

## Troubleshooting

**CAP value stays at 0 even when touching:**
- Confirm 10M ohm resistor is between pin 2 and pin 3 (not between pin 3 and the orb)
- Confirm wire goes to pin 3 (RECEIVE), not pin 2 (SEND)
- Check the orb is not grounded

**CAP value is huge and doesn't change with touch:**
- Orb is likely grounded somewhere; check all connections

**MIDI device not appearing in DAW:**
- Unplug and replug Teensy
- Confirm USB Type was set to MIDI before uploading (not just changed after)

**Display flashes LED instead of showing content:**
- OLED wiring issue, or wrong I2C address — change `OLED_ADDRESS` to `0x3D` and re-upload

**Readings are jittery:**
- Increase `CAP_SAMPLES` (try 50)
- Decrease `EMA_ALPHA` (try 0.1 for more smoothing)
- Add a 100–470 pF capacitor from pin 3 to GND to stabilize the receive line
