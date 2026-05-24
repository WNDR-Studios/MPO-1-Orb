// OrbMIDI — Teensy 4.1 Capacitive Touch MIDI Controller
//
// Senses a large metal orb via CapacitiveSensor library, outputs MIDI CC
// over USB, and shows live readings on a 128x32 SSD1306 OLED (I2C).
// A 5k potentiometer adjusts the sensitivity ceiling for environmental tuning.
//
// Arduino IDE setup:
//   Board  : Teensy 4.1
//   USB Type: MIDI  (Tools > USB Type > MIDI)
//
// Libraries required (install via Library Manager):
//   - CapacitiveSensor  by Paul Badger / Paul Stoffregen
//   - Adafruit SSD1306  (install with all dependencies)

#include <CapacitiveSensor.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------------
const int CAP_SEND_PIN    = 2;   // one side of the 10M ohm resistor
const int CAP_RECEIVE_PIN = 3;   // other side → 100 ohm series → orb
const int POT_PIN         = A0;  // 5k pot wiper; high lug to 3.3V, low to GND

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------
const int MIDI_CC_NUM  = 1;   // CC 1 = Modulation Wheel; change freely
const int MIDI_CHANNEL = 1;   // MIDI channel 1–16

// ---------------------------------------------------------------------------
// Capacitive sensor tuning
// ---------------------------------------------------------------------------
const int  CAP_SAMPLES   = 30;   // readings averaged per measurement; raise for stability, lower for speed
const long CAP_THRESHOLD = 20000;   // raw values below this = no touch; raise if ambient noise is high

// Pot maps to this range for the capacitive ceiling.
// After first upload, do a calibration pass (see CLAUDE.md) and adjust these.
const long CAP_MAX_LOW  = 10000;    // cap ceiling when pot is fully counter-clockwise (hair trigger)
const long CAP_MAX_HIGH = 4200000;   // cap ceiling when pot is fully clockwise (requires firm touch)

// Exponential moving average smoothing factor (0–1); lower = smoother but slower to respond
const float EMA_ALPHA = 0.5f;

// MIDI curve exponent — controls how cap values map to MIDI.
// 1.0 = linear; lower values (e.g. 0.3–0.5) spread proximity into more of the 0–127 range.
const float CURVE_EXPONENT = 0.4f;

// ---------------------------------------------------------------------------
// OLED
// ---------------------------------------------------------------------------
const int     SCREEN_WIDTH  = 128;
const int     SCREEN_HEIGHT = 32;
const int     OLED_RESET    = -1;     // no dedicated reset pin
const uint8_t OLED_ADDRESS  = 0x3C;

// ---------------------------------------------------------------------------
// Loop timing
// ---------------------------------------------------------------------------
const unsigned long LOOP_INTERVAL_MS = 20;  // ~50 Hz update rate

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
CapacitiveSensor cs(CAP_SEND_PIN, CAP_RECEIVE_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float         smoothedCap = 0.0f;
int           lastMidi    = -1;          // -1 forces the very first send
unsigned long lastLoopMs  = 0;

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Disable auto-calibration — keeps readings stable across sessions.
  cs.set_CS_AutocaL_Millis(0xFFFFFFFF);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // Blink built-in LED forever so a display wiring failure is obvious.
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(200);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(22, 12);
  display.print("ORB MIDI CTRL");
  display.display();
  delay(1500);
  display.clearDisplay();
  display.display();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Returns the current capacitive ceiling based on pot position.
long readCapMax() {
  int potVal = analogRead(POT_PIN);
  return map(potVal, 0, 1023, CAP_MAX_HIGH, CAP_MAX_LOW);
}

// Renders three data lines to the 128x32 OLED.
void updateDisplay(long capSmoothed, int midiVal, long capMax) {
  display.clearDisplay();

  display.setCursor(0, 0);
  display.print("CAP:  ");
  display.print(capSmoothed);

  display.setCursor(0, 11);
  display.print("MIDI: ");
  display.print(midiVal);

  display.setCursor(0, 22);
  display.print("MAX:  ");
  display.print(capMax);

  display.display();
}

// Sends MIDI CC only when the value has actually changed (prevents jitter floods).
void sendMidiIfChanged(int newVal) {
  if (newVal != lastMidi) {
    usbMIDI.sendControlChange(MIDI_CC_NUM, newVal, MIDI_CHANNEL);
    lastMidi = newVal;
  }
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  // Always drain the USB MIDI receive buffer — required for USB MIDI on Teensy.
  while (usbMIDI.read()) {}

  unsigned long now = millis();
  if (now - lastLoopMs < LOOP_INTERVAL_MS) return;
  lastLoopMs = now;

  // Read raw capacitance.
  long rawReading = cs.capacitiveSensor(CAP_SAMPLES);
  if (rawReading < 0) rawReading = 0;  // library can return -1 on the first call

  // Apply exponential moving average smoothing.
  smoothedCap = EMA_ALPHA * (float)rawReading + (1.0f - EMA_ALPHA) * smoothedCap;
  long capSmoothed = (long)smoothedCap;

  // Read pot → sensitivity ceiling.
  long capMax = readCapMax();

  // Apply no-touch threshold — anything below this is treated as silence.
  long effective = (capSmoothed < CAP_THRESHOLD) ? 0 : capSmoothed;

  // Map to MIDI 0–127 with a power curve so proximity spans more of the range.
  int midiVal = 0;
  if (effective > 0) {
    float normalized = (float)(effective - CAP_THRESHOLD) / (float)(capMax - CAP_THRESHOLD);
    normalized = constrain(normalized, 0.0f, 1.0f);
    midiVal = constrain((int)(pow(normalized, CURVE_EXPONENT) * 127.0f), 0, 127);
  }

  sendMidiIfChanged(midiVal);
  updateDisplay(capSmoothed, midiVal, capMax);
}
