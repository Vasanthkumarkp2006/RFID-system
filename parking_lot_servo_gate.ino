#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- BLUETOOTH --------
BluetoothSerial SerialBT;

// -------- SERVO --------
Servo gateServo;

// -------- PIN DEFINITIONS --------
#define SERVO_PIN 13
#define ENTRY_IR 4   // Gate IR sensor
#define BUZZER 2     // Buzzer pin

// Parking Slots
#define SLOT1 34
#define SLOT2 35
#define SLOT3 32
#define SLOT4 33

int totalSlots = 4;

// -------- VARIABLES --------
String command = "";
bool gateOpen = false;

unsigned long lastBeepTime = 0;
bool buzzerState = false;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_PARKING");

  // Sensor Pins
  pinMode(ENTRY_IR, INPUT);
  pinMode(SLOT1, INPUT);
  pinMode(SLOT2, INPUT);
  pinMode(SLOT3, INPUT);
  pinMode(SLOT4, INPUT);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Servo
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Closed

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Parking System");
  delay(2000);
  lcd.clear();

  Serial.println("System Ready...");
}

// -------- GATE FUNCTIONS --------
void openGate() {
  if (!gateOpen) {
    Serial.println("Opening Gate...");
    for (int pos = 0; pos <= 90; pos++) {
      gateServo.write(pos);
      delay(15);
    }
    gateOpen = true;
  }
}

void closeGate() {
  if (gateOpen) {
    Serial.println("Closing Gate...");
    for (int pos = 90; pos >= 0; pos--) {
      gateServo.write(pos);
      delay(15);
    }
    gateOpen = false;
  }
}

void loop() {

  // -------- READ SLOT STATUS --------
  int s1 = digitalRead(SLOT1);
  int s2 = digitalRead(SLOT2);
  int s3 = digitalRead(SLOT3);
  int s4 = digitalRead(SLOT4);

  int occupied = 0;

  if (s1 == LOW) occupied++;
  if (s2 == LOW) occupied++;
  if (s3 == LOW) occupied++;
  if (s4 == LOW) occupied++;

  int freeSlots = totalSlots - occupied;

  // -------- LCD DISPLAY --------
  lcd.setCursor(0, 0);
  lcd.print("Free Slots: ");
  lcd.print(freeSlots);
  lcd.print("   ");

  lcd.setCursor(0, 1);
  if (freeSlots == 0) {
    lcd.print("Parking FULL   ");
  } else {
    lcd.print("Welcome :)     ");
  }

  // -------- ENTRY IR SENSOR --------
  int irState = digitalRead(ENTRY_IR);

  // -------- BUZZER LOGIC --------
  if (freeSlots == 0 && irState == LOW) {
    // Beep every 200 ms
    if (millis() - lastBeepTime >= 200) {
      lastBeepTime = millis();
      buzzerState = !buzzerState;
      digitalWrite(BUZZER, buzzerState);
    }
    closeGate(); // Ensure gate stays closed
  } else {
    digitalWrite(BUZZER, LOW); // Turn off buzzer

    // Normal gate operation
    if (irState == LOW && freeSlots > 0) {
      openGate();
    } else if (irState == HIGH) {
      closeGate();
    }
  }

  // -------- BLUETOOTH CONTROL --------
  while (SerialBT.available()) {
    char c = SerialBT.read();
    command += c;
  }

  command.trim();

  if (command.length() > 0) {
    Serial.println("BT Command: " + command);

    if (command.equalsIgnoreCase("open") && freeSlots > 0) {
      openGate();
    }
    else if (command.equalsIgnoreCase("close")) {
      closeGate();
    }
    else if (command.equalsIgnoreCase("status")) {
      SerialBT.print("Free Slots: ");
      SerialBT.println(freeSlots);
    }

    command = "";
  }

  delay(50);
}
