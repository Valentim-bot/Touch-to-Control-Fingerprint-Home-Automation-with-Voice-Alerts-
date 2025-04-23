#include <Adafruit_Fingerprint.h>
#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // Include LCD I2C library

// Pin definitions
#define Green_LED   7
#define Yellow_LED  6
#define Solenoid    8

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x27 is a common I2C address. Use (0x3F, 16, 2) if your LCD has a different address

// Fingerprint sensor on Serial1 (Mega: TX1 = pin 18, RX1 = pin 19)
Adafruit_Fingerprint finger(&Serial1);

// 🔊 DFPlayer Mini on Serial2 (Mega: TX2 = pin 16, RX2 = pin 17)
DFRobotDFPlayerMini dfplayer;

unsigned long actionStartTime = 0;
bool actionInProgress = false;

void setup() {
  Serial.begin(115200);
  delay(100);

  //  Set up pins
  pinMode(Green_LED, OUTPUT);
  pinMode(Yellow_LED, OUTPUT);
  pinMode(Solenoid, OUTPUT);
  digitalWrite(Green_LED, LOW);
  digitalWrite(Yellow_LED, LOW);
  digitalWrite(Solenoid, HIGH);  // Keep solenoid locked at start

  //Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Place your finger");

  // Initialize fingerprint sensor
  Serial1.begin(57600);
  delay(5);

  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found.");
    lcd.setCursor(0, 1);
    lcd.print("Sensor OK       ");
  } else {
    Serial.println("Fingerprint sensor not found.");
    lcd.setCursor(0, 1);
    lcd.print("Sensor FAIL     ");
    while (1);  // Stop here if fingerprint not found
  }

  finger.getTemplateCount();
  Serial.print("Sensor contains ");
  Serial.print(finger.templateCount);
  Serial.println(" templates");

  // Initialize DFPlayer
  Serial2.begin(9600);
  if (!dfplayer.begin(Serial2)) {
    Serial.println("DFPlayer not found.");
    lcd.setCursor(0, 1);
    lcd.print("DFPlayer FAIL   ");
    while (1);  // Stop here if DFPlayer fails
  }
  Serial.println("DFPlayer Mini online.");
  lcd.setCursor(0, 1);
  lcd.print("DFPlayer OK     ");

  dfplayer.volume(30); // Max volume
}

void loop() {
  // Restore system after 3 seconds
  if (actionInProgress && millis() - actionStartTime >= 3000) {
    digitalWrite(Solenoid, HIGH);      // Lock again
    digitalWrite(Green_LED, LOW);
    digitalWrite(Yellow_LED, LOW);
    lcd.setCursor(0, 1);
    lcd.print("                "); // Clear line
    actionInProgress = false;
    lcd.setCursor(0, 0);
    lcd.print("Place your finger");
  }

  if (actionInProgress) return;

  // Check fingerprint
  uint8_t result = getFingerprintID();

  if (result == 1) {
    Serial.println("Authorized!");
    digitalWrite(Green_LED, HIGH);
    digitalWrite(Solenoid, LOW); // Unlock solenoid
    dfplayer.play(2); // Authorized sound
    lcd.setCursor(0, 0);
    lcd.print("Access Granted  ");
    lcd.setCursor(0, 1);
    lcd.print(" Welcome Home!         ");
  } 
  else if (result == 0xFF) {
    // No finger or scan error
    return;
  } 
  else {
    Serial.println("Unauthorized!");
    digitalWrite(Yellow_LED, HIGH);
    digitalWrite(Solenoid, HIGH); // Keep locked
    dfplayer.play(1); // Unauthorized sound
    lcd.setCursor(0, 0);
    lcd.print("Access Denied   ");
    lcd.setCursor(0, 1);
    lcd.print("Unauth.. Finger..       ");
  }

  // Start timeout
  actionStartTime = millis();
  actionInProgress = true;
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return 0xFF;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return 0xFF;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return 0xFE;

  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence "); Serial.println(finger.confidence);
  return 1; // Always return 1 to treat valid fingerprints as authorized
}
