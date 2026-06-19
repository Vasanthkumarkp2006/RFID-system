#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- TM1637 --------
#define CLK 18
#define DIO 19
TM1637Display display(CLK, DIO);

// -------- Buttons --------
#define BTN_NEXT 32
#define BTN_PREV 33

int totalDevices = 0;
int currentIndex = 0;

// Store SSIDs
String ssidList[50];   // max 50 networks

// -------- Timer --------
unsigned long lastScanTime = 0;
const unsigned long scanInterval = 20000; // 20 sec

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  lcd.init();
  lcd.backlight();

  display.setBrightness(5);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  // First scan
  scanNetworks();
  showDevice();

  lastScanTime = millis();
}

void loop() {

  // -------- Auto Scan Every 30 sec --------
  if (millis() - lastScanTime >= scanInterval) {
    scanNetworks();

    // Reset index safely
    currentIndex = 0;

    showDevice();

    lastScanTime = millis();
  }

  // -------- NEXT button --------
  if (digitalRead(BTN_NEXT) == LOW) {
    currentIndex++;
    if (currentIndex >= totalDevices) currentIndex = 0;
    showDevice();
    delay(300);
  }

  // -------- PREVIOUS button --------
  if (digitalRead(BTN_PREV) == LOW) {
    currentIndex--;
    if (currentIndex < 0) currentIndex = totalDevices - 1;
    showDevice();
    delay(300);
  }
}

// -------- Scan WiFi --------
void scanNetworks() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning...");

  int n = WiFi.scanNetworks();

  totalDevices = n;

  for (int i = 0; i < n && i < 50; i++) {
    ssidList[i] = WiFi.SSID(i);
  }

  Serial.println("Scan Done");

  // Show total count on TM1637
  display.showNumberDec(totalDevices, true);
}

// -------- Show Device --------
void showDevice() {

  lcd.clear();

  if (totalDevices == 0) {
    lcd.setCursor(0, 0);
    lcd.print("No Devices");
    return;
  }

  // Line 1 → Serial number
  lcd.setCursor(0, 0);
  lcd.print("Device: ");
  lcd.print(currentIndex + 1);

  // Line 2 → SSID
  lcd.setCursor(0, 1);
  lcd.print(ssidList[currentIndex]);

  Serial.print("Device ");
  Serial.print(currentIndex + 1);
  Serial.print(": ");
  Serial.println(ssidList[currentIndex]);
}
