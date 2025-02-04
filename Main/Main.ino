#include "arduino_config.h"
#include "LCDIC2.h"

LCDIC2 lcd(0x27, 16, 2);

void initializePins() {
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  pinMode(PUMP_DUNGER_PIN, OUTPUT);
  pinMode(PUMP_WATER_PIN, OUTPUT);
  pinMode(PUMP_ACID_PIN, OUTPUT);
  pinMode(PH_POWER, OUTPUT);
  pinMode(EC_POWER, OUTPUT);
  pinMode(PH_CALIBRATION_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PUMP_BUTTON_PIN, INPUT_PULLUP);
}

void setup() {
  Serial.begin(9600);
  initializePins();
  digitalWrite(PUMP_WATER_PIN, HIGH);  // Wasser soll immer eingeschaltet sein
  if (lcd.begin()) lcd.print("Growbox V1.0");
  lcd.setCursor(0, 1);
  lcd.print("Bitte warten...");
  delay(1000);
}

void loop() {
  regulateLight();
  checkPumpState();
  updatePh();
  updateEc();
  delay(1000);
}

void checkPumpState() {
  currentPumpState = (digitalRead(PUMP_BUTTON_PIN) == pumpsInactiveState) ? false : true;
  String supplyPumpStatusText = currentPumpState ? "Pumpen aktiv     " : "Pumpen inaktiv   ";
  Serial.println(supplyPumpStatusText);
  lcd.setCursor(0, 1);
  lcd.print(supplyPumpStatusText);
}

// pH calibration function
void calibratePhIfRequested() {
  int calibrationLiquidValue = 7;
  currentPhCalibrationButtonState = digitalRead(PH_CALIBRATION_BUTTON_PIN);
  if ((lastPhCalibrationButtonState == HIGH && currentPhCalibrationButtonState == LOW) || (lastPhCalibrationButtonState == LOW && currentPhCalibrationButtonState == HIGH)) {
    lcd.setCursor(0, 0);
    lcd.print("Kalibrierung...    ");
    delay(1000); 

    phCalibrationOffset = 7 - (mapfloat(analogRead(PH_PIN), ph4ReadValue, ph7ReadValue, 4.01, 7.01));
    ph7ReadValue = analogRead(PH_PIN);

    lcd.setCursor(0, 0);
    lcd.print("Offset: ");
    lcd.print(String(phCalibrationOffset));
    delay(1000);
  }

  lastPhCalibrationButtonState = currentPhCalibrationButtonState;
}


// pH regulation function
void updatePh() {
  digitalWrite(EC_POWER, LOW); //PH und EC dürfen nie gleichzeitig strom haben
  delay(1000);
  digitalWrite(PH_POWER, HIGH);
  delay(1000);
  calibratePhIfRequested();
  currentPhValue = mapfloat(analogRead(PH_PIN), ph4ReadValue, ph7ReadValue, 4.01, 7.01);

  lcd.setCursor(0, 0);
  lcd.print("PH ");
  lcd.print(String(currentPhValue, 2));
  lcd.print(" ");
  Serial.print("PH Wert: ");
  Serial.println(currentPhValue, 2);

  static unsigned long lastAcidPumpMillis = 0;
  if (currentPumpState && currentPhValue > TARGET_PH_HIGH && millis() - lastAcidPumpMillis > 60000) {  // 60 seconds delay
    digitalWrite(PUMP_ACID_PIN, HIGH);
    delay(3000);
    digitalWrite(PUMP_ACID_PIN, LOW);
    lastAcidPumpMillis = millis();
  }
}

//Manuell Gemessene Werte umwandeln
float mapfloat(long x, long in_min, long in_max, float out_min, float out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}


void updateEc() {
  digitalWrite(PH_POWER, LOW);
  delay(1000);
  digitalWrite(EC_POWER, HIGH);
  delay(1000);
  int sensorValueEC = analogRead(EC_PIN);
  float voltageEC = sensorValueEC * (5.0 / 1023.0);
  float EC = voltageEC;

  lcd.print("EC: ");
  lcd.print(String(EC, 2));
  lcd.print("    ");
  Serial.print("EC Wert: ");
  Serial.println(EC, 2);

static unsigned long lastDungerPumpMillis = 0;
 Serial.println((millis() - lastDungerPumpMillis));
  if (currentPumpState && EC < TARGET_EC && (millis() - lastDungerPumpMillis) > 60000) {
    digitalWrite(PUMP_DUNGER_PIN, HIGH);
    delay(5000);  
    digitalWrite(PUMP_DUNGER_PIN, LOW);
    lastDungerPumpMillis = millis();
  }
}

void regulateLight() {
  if ((lightState && millis() - lightChangeMillis >= LIGHT_ON_HOURS * 3600000UL) || (!lightState && millis() - lightChangeMillis >= LIGHT_OFF_HOURS * 3600000UL)) {
    lightState = !lightState;
    digitalWrite(LIGHT_RELAY_PIN, lightState ? HIGH : LOW);
    lightChangeMillis = millis();
  }
}