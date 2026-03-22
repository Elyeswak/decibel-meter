
#include <LiquidCrystal.h>

LiquidCrystal lcd(    7,  8,  9, 10, 11, 12);

#define SENSOR_PIN A0
#define PIN_QUIET 3     
#define PIN_MODERATE 4  
#define PIN_LOUD 5  

const int sampleWindow = 50;
unsigned int sample;


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

  lcd.setCursor(0, 0);
  lcd.print("Sound Level");
  lcd.setCursor(0, 1);
  lcd.print("Meter v1.0");
  delay(2000);
  lcd.clear();
}

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
  int db = map(peakToPeak, 20, 900, 49, 90);


  lcd.setCursor(0, 0);
  lcd.print("Loudness: ");
  lcd.print(db);
  lcd.print(" dB  ");


  if (db <= 60) {
    lcd.setCursor(0, 1);
    lcd.print("Level: Quiet   ");
    digitalWrite(PIN_QUIET, HIGH);
    digitalWrite(PIN_MODERATE, LOW);
    digitalWrite(PIN_LOUD, LOW);
  }
  else if (db > 60 && db < 85) {
    lcd.setCursor(0, 1);
    lcd.print("Level: Moderate");
    digitalWrite(PIN_QUIET, LOW);
    digitalWrite(PIN_MODERATE, HIGH);
    digitalWrite(PIN_LOUD, LOW);
  }
  else if (db >= 85) {
    lcd.setCursor(0, 1);
    lcd.print("Level: LOUD!   ");
    digitalWrite(PIN_QUIET, LOW);
    digitalWrite(PIN_MODERATE, LOW);
    digitalWrite(PIN_LOUD, HIGH);
  }

  Serial.print("Peak: ");
  Serial.print(peakToPeak);
  Serial.print(" | dB: ");
  Serial.println(db);

  delay(200);
  lcd.clear();
}
