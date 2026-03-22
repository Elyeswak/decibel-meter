#include <LiquidCrystal.h>

LiquidCrystal lcd(    7,  8,  9, 10, 11, 12);

#define SENSOR_PIN A0
#define PIN_QUIET 3
#define PIN_MODERATE 4
#define PIN_LOUD 5

const int sampleWindow = 50;
unsigned int sample;

const float ADC_REF = 20.0;  )
const float DB_MIN  = 40.0;  
const float DB_MAX  = 95.0;   

byte bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
byte bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
byte bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};
byte bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
byte bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

const int SMOOTH_COUNT = 3;
float dbHistory[3] = {40.0, 40.0, 40.0};
int historyIndex = 0;

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

  lcd.createChar(0, bar1);
  lcd.createChar(1, bar2);
  lcd.createChar(2, bar3);
  lcd.createChar(3, bar4);
  lcd.createChar(4, bar5);

  lcd.setCursor(0, 0);
  lcd.print(" Sound  Level");
  lcd.setCursor(0, 1);
  lcd.print(" Meter  v2.0");
  delay(2000);
  lcd.clear();
}

// ==================== MAIN LOOP ====================
void loop() {
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


  float peakToPeak = signalMax - signalMin;

  float db;
  if (peakToPeak <= ADC_REF) {
    db = DB_MIN;
  } else {
    db = DB_MIN + 20.0 * log10(peakToPeak / ADC_REF);
  }

  if (db < DB_MIN) db = DB_MIN;
  if (db > DB_MAX) db = DB_MAX;

  dbHistory[historyIndex] = db;
  historyIndex = (historyIndex + 1) % SMOOTH_COUNT;

  float dbSmoothed = 0;
  for (int i = 0; i < SMOOTH_COUNT; i++) {
    dbSmoothed += dbHistory[i];
  }
  dbSmoothed /= SMOOTH_COUNT;

  int dbDisplay = (int)dbSmoothed;

  lcd.setCursor(0, 0);

  if (dbDisplay <= 60) {
    lcd.print("Quiet  ");
  } else if (dbDisplay < 85) {
    lcd.print("Medium ");
  } else {
    lcd.print("LOUD!  ");
  }

  lcd.setCursor(10, 0);
  if (dbDisplay < 10) lcd.print("  ");
  else if (dbDisplay < 100) lcd.print(" ");
  lcd.print(dbDisplay);
  lcd.print(" dB");

  drawBar(dbSmoothed);

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

void drawBar(float db) {
  int barLength = map((int)db, (int)DB_MIN, (int)DB_MAX, 0, 80);
  barLength = constrain(barLength, 0, 80);

  int fullBlocks = barLength / 5;  
  int remainder  = barLength % 5; 

  lcd.setCursor(0, 1);

  for (int i = 0; i < fullBlocks && i < 16; i++) {
    lcd.write(byte(4));
  }

  if (remainder > 0 && fullBlocks < 16) {
    lcd.write(byte(remainder - 1));
    fullBlocks++;
  }

  for (int i = fullBlocks; i < 16; i++) {
    lcd.print(" ");
  }
}