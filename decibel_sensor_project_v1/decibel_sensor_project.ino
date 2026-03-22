/*
 * Sound Level Meter v2.0 — Logarithmic dB + LCD Bar Graph
 * 
 * Reads analog sound sensor, calculates decibel level using
 * the actual logarithmic formula (not linear mapping), and
 * displays both the dB number and a real-time bar graph
 * on a 16x2 LCD using custom characters.
 * 
 * Hardware: Arduino Uno R3, 16x2 LCD (parallel), Analog Sound Sensor, 3 LEDs
 * 
 * LCD Wiring:
 *   RS → D7  |  E → D8  |  R/W → GND
 *   DB4 → D9  |  DB5 → D10  |  DB6 → D11  |  DB7 → D12
 *   V0 → Potentiometer  |  VSS → GND  |  VDD → 5V
 * 
 * Sound Sensor: AO → A0  |  VCC → 5V  |  GND → GND
 * LEDs: Green → D3  |  Yellow → D4  |  Red → D5
 */

#include <LiquidCrystal.h>

// ==================== LCD SETUP ====================
//                   RS  E  DB4 DB5 DB6 DB7
LiquidCrystal lcd(    7,  8,  9, 10, 11, 12);

// ==================== PIN DEFINITIONS ====================
#define SENSOR_PIN A0
#define PIN_QUIET 3
#define PIN_MODERATE 4
#define PIN_LOUD 5

// ==================== SAMPLING CONFIG ====================
const int sampleWindow = 50;   // 50 ms window (captures down to 20 Hz)
unsigned int sample;

// ==================== dB CALIBRATION ====================
// These values depend on your specific sensor. Adjust if needed.
// ADC_REF is the peak-to-peak value in a silent room (sensor noise floor)
// ADC_MAX is the peak-to-peak value at the loudest sound you expect
// DB_MIN corresponds to ADC_REF, DB_MAX corresponds to ADC_MAX
const float ADC_REF = 20.0;    // Noise floor of sensor (~quiet room)
const float DB_MIN  = 40.0;    // dB at noise floor (quiet room ~40 dB)
const float DB_MAX  = 95.0;    // dB at maximum sensor reading

// ==================== CUSTOM LCD BAR CHARACTERS ====================
// Each custom character fills a different number of columns (1 to 5)
// This gives us 5 levels per LCD cell × 16 cells = 80 levels of resolution
// Way more precise than just printing a number!

byte bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};  // █
byte bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};  // ██
byte bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};  // ███
byte bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};  // ████
byte bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};  // █████ (full block)

// ==================== SMOOTHING ====================
// Average the last few readings to reduce jitter
const int SMOOTH_COUNT = 3;
float dbHistory[3] = {40.0, 40.0, 40.0};
int historyIndex = 0;

// ==================== SETUP ====================
void setup() {
  pinMode(SENSOR_PIN, INPUT);
  pinMode(PIN_QUIET, OUTPUT);
  pinMode(PIN_MODERATE, OUTPUT);
  pinMode(PIN_LOUD, OUTPUT);

  digitalWrite(PIN_QUIET, LOW);
  digitalWrite(PIN_MODERATE, LOW);
  digitalWrite(PIN_LOUD, LOW);

  Serial.begin(9600);

  lcd.begin(16, 2);

  // Load custom bar characters into LCD memory (slots 0-4)
  lcd.createChar(0, bar1);
  lcd.createChar(1, bar2);
  lcd.createChar(2, bar3);
  lcd.createChar(3, bar4);
  lcd.createChar(4, bar5);

  // Startup message
  lcd.setCursor(0, 0);
  lcd.print(" Sound  Level");
  lcd.setCursor(0, 1);
  lcd.print(" Meter  v2.0");
  delay(2000);
  lcd.clear();
}

// ==================== MAIN LOOP ====================
void loop() {
  // ---- STEP 1: SAMPLE AUDIO (50ms window) ----
  unsigned long startMillis = millis();
  unsigned int signalMax = 0;
  unsigned int signalMin = 1024;

  while (millis() - startMillis < sampleWindow) {
    sample = analogRead(SENSOR_PIN);
    if (sample < 1024) {
      if (sample > signalMax) signalMax = sample;
      else if (sample < signalMin) signalMin = sample;
    }
  }

  // ---- STEP 2: CALCULATE dB USING LOGARITHMIC FORMULA ----
  //
  // The actual decibel formula is:  dB = 20 * log10(P / P_ref)
  //
  // We can't measure real Pascals with this sensor, but we CAN
  // apply the same logarithmic relationship to our ADC values.
  // The ratio (peakToPeak / ADC_REF) represents how many times
  // louder the current sound is compared to the quiet reference.
  //
  // This is MORE ACCURATE than linear mapping because human hearing
  // and the dB scale are both logarithmic — doubling the sound
  // pressure adds ~6 dB, not a fixed amount.

  float peakToPeak = signalMax - signalMin;

  float db;
  if (peakToPeak <= ADC_REF) {
    db = DB_MIN;  // At or below noise floor
  } else {
    // Core formula: dB = 20 * log10(measured / reference)
    // Then offset by DB_MIN to shift into real-world dB range
    db = DB_MIN + 20.0 * log10(peakToPeak / ADC_REF);
  }

  // Clamp to reasonable range
  if (db < DB_MIN) db = DB_MIN;
  if (db > DB_MAX) db = DB_MAX;

  // ---- STEP 3: SMOOTH THE READING ----
  // Average last 3 readings to reduce flickering on the LCD
  dbHistory[historyIndex] = db;
  historyIndex = (historyIndex + 1) % SMOOTH_COUNT;

  float dbSmoothed = 0;
  for (int i = 0; i < SMOOTH_COUNT; i++) {
    dbSmoothed += dbHistory[i];
  }
  dbSmoothed /= SMOOTH_COUNT;

  int dbDisplay = (int)dbSmoothed;

  // ---- STEP 4: DISPLAY dB VALUE ON TOP ROW ----
  lcd.setCursor(0, 0);

  // Show level label + dB number
  if (dbDisplay <= 60) {
    lcd.print("Quiet  ");
  } else if (dbDisplay < 85) {
    lcd.print("Medium ");
  } else {
    lcd.print("LOUD!  ");
  }

  // Right-align the dB number
  lcd.setCursor(10, 0);
  if (dbDisplay < 10) lcd.print("  ");
  else if (dbDisplay < 100) lcd.print(" ");
  lcd.print(dbDisplay);
  lcd.print(" dB");

  // ---- STEP 5: DRAW BAR GRAPH ON BOTTOM ROW ----
  drawBar(dbSmoothed);

  // ---- STEP 6: LIGHT THE APPROPRIATE LED ----
  if (dbDisplay <= 60) {
    digitalWrite(PIN_QUIET, HIGH);
    digitalWrite(PIN_MODERATE, LOW);
    digitalWrite(PIN_LOUD, LOW);
  } else if (dbDisplay < 85) {
    digitalWrite(PIN_QUIET, LOW);
    digitalWrite(PIN_MODERATE, HIGH);
    digitalWrite(PIN_LOUD, LOW);
  } else {
    digitalWrite(PIN_QUIET, LOW);
    digitalWrite(PIN_MODERATE, LOW);
    digitalWrite(PIN_LOUD, HIGH);
  }

  // ---- STEP 7: SERIAL OUTPUT FOR DEBUGGING ----
  Serial.print("Raw P2P: ");
  Serial.print(peakToPeak);
  Serial.print(" | dB (log): ");
  Serial.print(dbSmoothed, 1);
  Serial.print(" | Level: ");
  if (dbDisplay <= 60) Serial.println("Quiet");
  else if (dbDisplay < 85) Serial.println("Moderate");
  else Serial.println("LOUD!");

  delay(100);
}

// ==================== BAR GRAPH FUNCTION ====================
// Draws a horizontal bar across the bottom row of the LCD
// that grows and shrinks with the dB level.
//
// Each LCD cell is 5 pixels wide. With 16 cells on the bottom row,
// we have 16 × 5 = 80 pixel columns of resolution.
// The 5 custom characters (bar1 to bar5) each fill 1 to 5 columns.

void drawBar(float db) {
  // Map dB range to bar length (0 to 80 pixel columns)
  int barLength = map((int)db, (int)DB_MIN, (int)DB_MAX, 0, 80);
  barLength = constrain(barLength, 0, 80);

  int fullBlocks = barLength / 5;    // Number of fully filled cells
  int remainder  = barLength % 5;    // Partial fill for the last cell

  lcd.setCursor(0, 1);

  // Draw full blocks
  for (int i = 0; i < fullBlocks && i < 16; i++) {
    lcd.write(byte(4));  // Full block character (bar5)
  }

  // Draw partial block (if there's a remainder and space left)
  if (remainder > 0 && fullBlocks < 16) {
    lcd.write(byte(remainder - 1));  // bar1=0, bar2=1, bar3=2, bar4=3
    fullBlocks++;  // Count this cell as used
  }

  // Clear the remaining cells
  for (int i = fullBlocks; i < 16; i++) {
    lcd.print(" ");
  }
}