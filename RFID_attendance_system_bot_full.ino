#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>
#include <EEPROM.h>
#include <RTClib.h>

// -------- WIFI --------
const char* ssids[] = {
  "VK",
  "AndroidAP_6391_lb",
  "realme 10 Pro 5G",
  "realme P1 5G"
};

const char* passwords[] = {
  "vasanthkumar",
  "123456789",
  "12341234",
  "shibi_14"
};

const int wifiCount = 4;

// -------- TELEGRAM --------
#define BOT_TOKEN "8754649794:AAFX7VJiKjMfQZ1vWu_9-YzuakOzbSLFq4U"
#define CHAT_ID "7800568493"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// -------- RTC --------
RTC_DS1307 rtc;

// -------- RFID --------
#define SS_PIN 5
#define RST_PIN 16
MFRC522 rfid(SS_PIN, RST_PIN);

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------- TM1637 --------
#define CLK 4
#define DIO 15
TM1637Display display(CLK, DIO);

// -------- BUZZER --------
#define BUZZER 2

// -------- IR SENSOR --------
#define IR_SENSOR 34

// -------- BUTTONS --------
#define BTN_MODE   32
#define BTN_NEXT   25
#define BTN_PREV   33
#define BTN_RESET  26
#define BTN_EXPORT 27
#define BTN_SYNC   14
#define BTN_TOTAL  12

// -------- EEPROM --------
#define EEPROM_SIZE 512

// -------- STRUCT --------
struct Student {
  String cardID;
  String name;
  String roll;
  String college;
  String payment;
  String type;
  String department;
  String time;
};

// -------- STUDENTS --------
Student students[] = {
  {"93A2502D","VK key 1","1111","KEC","PAID","STUDENT","CSE",""},
  {"608CE4","Vasanth Kumar K P","23ECR231","KEC","PAID","STUDENT","ECE",""},
  {"A4ADB6","Person 1","123","KPC","NOT PAID","FACULTY","EEE",""},
  {"8C9CDC6","Person 2","456","KPC","PAID","FACULTY","ECE",""},
  {"594D636","KEY 2","159","KEC","NOT PAID","STUDENT","MECH",""},
  {"1D6CE06","Person 3","357","KPC","PAID","STUDENT","EIE",""}
};

int totalStudents = sizeof(students) / sizeof(students[0]);

bool marked[100] = {false};

int presentCount = 0;
int wrongCount = 0;

String wrongCards[100];
String wrongCardTimes[100];

int wrongCardCount = 0;

// -------- LOG --------
int logAddress = 100;
int logCount = 0;

// -------- MANUAL MODE --------
bool manualMode = false;
int viewIndex = 0;

// -------- IR --------
int irCount = 0;
bool lastIRState = HIGH;

// -------- ALERT TIMER --------
unsigned long mismatchStartTime = 0;
bool mismatchActive = false;

// -------- TOTAL MODE --------
bool totalMode = false;

// -------- BUZZER --------
void beep(int t) {

  for (int i = 0; i < t; i++) {

    digitalWrite(BUZZER, HIGH);
    delay(200);

    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}

// -------- TIME --------
String getTimeNow() {

  DateTime now = rtc.now() - TimeSpan(0, 0, 4, 0); // Subtract 4 minutes

  int hour = now.hour();

  String ampm = "AM";

  if (hour >= 12)
    ampm = "PM";

  if (hour == 0)
    hour = 12;

  else if (hour > 12)
    hour -= 12;

  char buf[20];

  sprintf(buf,
          "%02d:%02d:%02d %s",
          hour,
          now.minute(),
          now.second(),
          ampm.c_str());

  return String(buf);
}

String getDateNow() {

  DateTime now = rtc.now();

  char buf[15];

  sprintf(buf,
          "%02d/%02d/%04d",
          now.day(),
          now.month(),
          now.year());

  return String(buf);
}

// -------- SAVE LOG --------
void saveLog(int index) {

  DateTime now = rtc.now();

  EEPROM.write(logAddress++, index);
  EEPROM.write(logAddress++, now.hour());
  EEPROM.write(logAddress++, now.minute());
  EEPROM.write(logAddress++, now.day());
  EEPROM.write(logAddress++, now.month());

  logCount++;

  EEPROM.write(2, logCount);

  EEPROM.commit();
}

// -------- SAVE STUDENT TIME --------
void saveStudentTime(int index, String timeNow) {

  int addr = 250 + (index * 20);

  for (int i = 0; i < timeNow.length(); i++) {
    EEPROM.write(addr + i, timeNow[i]);
  }

  EEPROM.write(addr + timeNow.length(), '\0');

  EEPROM.commit();
}

// -------- SAVE WRONG CARD --------
void saveWrongCard(String id, String timeNow) {

  int addr = 400 + (wrongCardCount * 20);

  for (int i = 0; i < id.length(); i++) {
    EEPROM.write(addr + i, id[i]);
  }

  EEPROM.write(addr + id.length(), '\0');

  int timeAddr = addr + 10;

  for (int i = 0; i < timeNow.length(); i++) {
    EEPROM.write(timeAddr + i, timeNow[i]);
  }

  EEPROM.write(timeAddr + timeNow.length(), '\0');

  EEPROM.commit();
}

// -------- LOAD --------
void loadData() {

  wrongCount = EEPROM.read(0);
  presentCount = EEPROM.read(1);
  logCount = EEPROM.read(2);

  logAddress = 100 + (logCount * 5);

  for (int i = 0; i < totalStudents; i++) {
    marked[i] = EEPROM.read(i + 3);
  }

  // Load student scan times
  for (int i = 0; i < totalStudents; i++) {

    char timeBuffer[20] = {0};

    int addr = 250 + (i * 20);

    for (int j = 0; j < 19; j++) {

      timeBuffer[j] = EEPROM.read(addr + j);

      if (timeBuffer[j] == '\0')
        break;
    }

    students[i].time = String(timeBuffer);
  }

  wrongCardCount = wrongCount;

  // Load wrong card details
  for (int i = 0; i < wrongCardCount; i++) {

    int addr = 400 + (i * 20);

    char idBuffer[12] = {0};
    char timeBuffer[15] = {0};

    for (int j = 0; j < 10; j++) {

      idBuffer[j] = EEPROM.read(addr + j);

      if (idBuffer[j] == '\0')
        break;
    }

    for (int j = 0; j < 14; j++) {

      timeBuffer[j] = EEPROM.read(addr + 10 + j);

      if (timeBuffer[j] == '\0')
        break;
    }

    wrongCards[i] = String(idBuffer);
    wrongCardTimes[i] = String(timeBuffer);
  }
}

// -------- SAVE --------
void saveData() {

  EEPROM.write(0, wrongCount);
  EEPROM.write(1, presentCount);

  for (int i = 0; i < totalStudents; i++) {
    EEPROM.write(i + 3, marked[i]);
  }

  EEPROM.commit();
}

// -------- RESET --------
void resetData() {

  presentCount = 0;
  wrongCount = 0;
  wrongCardCount = 0;

  logCount = 0;
  logAddress = 100;

  irCount = 0;

  for (int i = 0; i < totalStudents; i++) {

    marked[i] = false;
    students[i].time = "";
  }

  for (int i = 0; i < 100; i++) {

    wrongCards[i] = "";
    wrongCardTimes[i] = "";
  }

  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }

  EEPROM.commit();

  lcd.clear();
  lcd.print("Memory Cleared");

  digitalWrite(BUZZER, HIGH);
  delay(3000);

  digitalWrite(BUZZER, LOW);
  delay(150);

  digitalWrite(BUZZER, HIGH);
  delay(150);

  digitalWrite(BUZZER, LOW);
  delay(150);

  digitalWrite(BUZZER, HIGH);
  delay(150);

  digitalWrite(BUZZER, LOW);

  lcd.clear();
  lcd.print("Scan your card");
}

// -------- RFID --------
String getCardID() {

  String id = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    id += String(rfid.uid.uidByte[i], HEX);
  }

  id.toUpperCase();

  return id;
}

int findStudent(String id) {

  for (int i = 0; i < totalStudents; i++) {

    if (students[i].cardID == id)
      return i;
  }

  return -1;
}

bool isWrongCardAlreadyScanned(String id) {

  for (int i = 0; i < wrongCardCount; i++) {

    if (wrongCards[i] == id)
      return true;
  }

  return false;
}

// -------- MANUAL VIEW --------
void showStudent(int index) {

  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print(students[index].name.substring(0,16));

  lcd.setCursor(0,1);
  lcd.print(students[index].roll);
}

// -------- WIFI CONNECTION --------
// -------- WIFI --------
bool connectWiFiNow() {

  client.setInsecure();

  for (int i = 0; i < wifiCount; i++) {

    lcd.clear();
    lcd.print("Trying WiFi");
    lcd.setCursor(0, 1);
    lcd.print(i + 1);

    WiFi.begin(ssids[i], passwords[i]);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 8000) {

      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {

      Serial.println("Connected:");
      Serial.println(ssids[i]);

      lcd.clear();
      lcd.print("Connected");
      lcd.setCursor(0,1);
      lcd.print(ssids[i]);

      delay(1000);

      return true;
    }

    WiFi.disconnect(true);
    delay(500);
  }

  return false;
}

// -------- TELEGRAM --------
void sendToTelegram() {

  String msg = "DATE: " + getDateNow() + "\n\n";

  int kecCount = 0;
  int kpcCount = 0;

  int paidCount = 0;
  int notPaidCount = 0;

  int kecPaid = 0;
  int kecNotPaid = 0;

  int kpcPaid = 0;
  int kpcNotPaid = 0;

  int studentCount = 0;
  int facultyCount = 0;

  String notPaidList = "";

  for (int i = 0; i < totalStudents; i++) {

    if (marked[i]) {

      msg += students[i].name + " - ";
      msg += students[i].roll;

      msg += " (" + students[i].college + ", ";
      msg += students[i].department + ", ";
      msg += students[i].payment + ")";

      msg += " (" + students[i].time + ")\n";

      if (students[i].payment == "NOT PAID") {

        notPaidList += students[i].name + " - ";
        notPaidList += students[i].roll;

        notPaidList += " (" + students[i].college + ")";
        notPaidList += " (" + students[i].time + ")\n";
      }
      if (students[i].type == "STUDENT")
        studentCount++;

      else if (students[i].type == "FACULTY")
        facultyCount++;

      if (students[i].college == "KEC") {

        kecCount++;

        if (students[i].payment == "PAID")
          kecPaid++;
        else
          kecNotPaid++;
      }

      else if (students[i].college == "KPC") {

        kpcCount++;

        if (students[i].payment == "PAID")
          kpcPaid++;
        else
          kpcNotPaid++;
      }

      if (students[i].payment == "PAID")
        paidCount++;
      else
        notPaidCount++;
    }
  }

  msg += "\nNOT PAID LIST:\n";

  if (notPaidList == "")
    msg += "None\n";
  else
    msg += notPaidList;

  msg += "\nWRONG CARD IDs:\n";

  if (wrongCardCount == 0) {

    msg += "None\n";
  }

  else {

    for (int i = 0; i < wrongCardCount; i++) {

      msg += String(i + 1) + ". ";

      msg += wrongCards[i];

      msg += " (";
      msg += wrongCardTimes[i];
      msg += ")";

      msg += "\n";
    }
  }

  msg += "\n\nSTUDENTS: " + String(studentCount);
  msg += "\nFACULTY: " + String(facultyCount);

  msg += "\n\nKEC: " + String(kecCount);
  msg += "\nKPC: " + String(kpcCount);  

  msg += "\n\nKEC PAID: " + String(kecPaid);
  msg += "\nKEC NOT PAID: " + String(kecNotPaid);

  msg += "\n\nKPC PAID: " + String(kpcPaid);
  msg += "\nKPC NOT PAID: " + String(kpcNotPaid);

  msg += "\n\nPAID: " + String(paidCount);
  msg += "\nNOT PAID: " + String(notPaidCount);

  msg += "\n\nTOTAL: " + String(presentCount);
  msg += "\nWRONG: " + String(wrongCount);

  msg += "\n\nGRAND TOTAL: ";
  msg += String(presentCount + wrongCount);

  bot.sendMessage(CHAT_ID, msg, "");
}

// -------- DISPLAY --------
void updateDisplay() {

  // TOTAL MODE
  if (totalMode) {

    int total = presentCount + wrongCount;

    display.showNumberDec(total, true);
  }

  // NORMAL MODE
  // TOTAL COUNT DETAILS
  else {

    int value = wrongCount * 100 + presentCount;

    display.showNumberDecEx(value,
                            0b01000000,
                            true);
  }
}

// -------- SETUP --------
void setup() {

  Serial.begin(115200);

  Wire.begin();

  rtc.begin();

  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();

  display.setBrightness(5);

  pinMode(BUZZER, OUTPUT);

  pinMode(IR_SENSOR, INPUT);

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_EXPORT, INPUT_PULLUP);
  pinMode(BTN_SYNC, INPUT_PULLUP);
  pinMode(BTN_TOTAL, INPUT_PULLUP);

  EEPROM.begin(EEPROM_SIZE);

  loadData();

  lcd.print("Scan your card");
}

// -------- LOOP --------
void loop() {

  bool currentIR = digitalRead(IR_SENSOR);

  if (lastIRState == HIGH &&
      currentIR == LOW) {

    irCount++;

    delay(300);
  }

  lastIRState = currentIR;

  // -------- MODE BUTTON --------
  if (digitalRead(BTN_MODE) == LOW) {

    manualMode = !manualMode;

    delay(300);

    if (manualMode) {

      if (presentCount == 0) {

        lcd.clear();
        lcd.print("No entry");

        delay(1000);

        manualMode = false;

        lcd.clear();
        lcd.print("Scan your card");

        return;
      }

      for (int i = 0; i < totalStudents; i++) {

        if (marked[i]) {

          viewIndex = i;
          break;
        }
      }

      showStudent(viewIndex);
    }

    else {

      lcd.clear();
      lcd.print("Scan your card");
    }
  }

  // -------- MANUAL VIEW --------
  if (manualMode) {

    if (digitalRead(BTN_NEXT) == LOW) {

      do {
        viewIndex = (viewIndex + 1) % totalStudents;
      }

      while (!marked[viewIndex]);

      showStudent(viewIndex);

      delay(300);
    }

    if (digitalRead(BTN_PREV) == LOW) {

      do {
        viewIndex = (viewIndex - 1 + totalStudents) % totalStudents;
      }

      while (!marked[viewIndex]);

      showStudent(viewIndex);

      delay(300);
    }

    return;
  }

  // -------- SYNC --------
  if (digitalRead(BTN_SYNC) == LOW) {

    irCount = presentCount + wrongCount;

    mismatchActive = false;

    digitalWrite(BUZZER, LOW);

    lcd.clear();
    lcd.print("Synced!");

    beep(2);

    delay(1000);

    lcd.clear();
    lcd.print("Scan your card");

    return;
  }

  // -------- EXPORT --------
  if (digitalRead(BTN_EXPORT) == LOW) {

    lcd.clear();
    lcd.print("Connecting WiFi");

    if (connectWiFiNow()) {

      lcd.clear();
      lcd.print("Sending Data");

      sendToTelegram();

      beep(2);
    }

    else {

      lcd.clear();
      lcd.print("WiFi Failed");

      beep(3);
    }

    WiFi.disconnect(true);

    lastIRState = digitalRead(IR_SENSOR);
    mismatchActive = false;
    digitalWrite(BUZZER, LOW);

    delay(1500);

    lcd.clear();
    lcd.print("Scan your card");

    return;
  }

  // -------- RESET --------
  if (digitalRead(BTN_RESET) == LOW) {

    resetData();

    return;
  }

  updateDisplay();

  // -------- TOTAL DISPLAY TOGGLE --------
  if (digitalRead(BTN_TOTAL) == LOW) {

    totalMode = !totalMode;

    lcd.clear();

    // TOTAL MODE
    if (totalMode) {

      int total = presentCount + wrongCount;

      lcd.print("TOTAL COUNT");

      lcd.setCursor(0,1);
      lcd.print(total);
    }

    // NORMAL MODE
    else {

      lcd.print("NORMAL MODE");

      lcd.setCursor(0,1);
      lcd.print("WRONG:CORRECT");
    }

    beep(1);

    delay(1000);

    lcd.clear();
    lcd.print("Scan your card");

    updateDisplay();

    delay(300);

    return;
  }

  // -------- IR ALERT --------
  if (irCount > (presentCount + wrongCount)) {

    if (!mismatchActive) {

      mismatchStartTime = millis();

      mismatchActive = true;
    }

    if (millis() - mismatchStartTime >= 10000) {

      lcd.setCursor(0,0);
      lcd.print("NOT SCANNED");

      digitalWrite(BUZZER, HIGH);
      delay(500);

      digitalWrite(BUZZER, LOW);
      delay(500);
    }
  }

  else {

    mismatchActive = false;

    digitalWrite(BUZZER, LOW);
  }

  // -------- RFID --------
  if (!rfid.PICC_IsNewCardPresent() ||
      !rfid.PICC_ReadCardSerial())
    return;

  String id = getCardID();

  int index = findStudent(id);

  lcd.clear();

  // -------- VALID CARD --------
  if (index != -1) {

    if (!marked[index]) {

      marked[index] = true;

      presentCount++;

      students[index].time = getTimeNow();

      saveStudentTime(index,
                      students[index].time);

      saveLog(index);

      saveData();

      lcd.print("WELCOME");

      lcd.setCursor(0,1);
      lcd.print(students[index].roll);

      beep(1);
    }

    else {

      lcd.print("Already Marked");

      beep(2);
    }
  }

  // -------- WRONG CARD --------
  else {

    if (!isWrongCardAlreadyScanned(id)) {

      String wrongTime = getTimeNow();

      wrongCards[wrongCardCount] = id;

      wrongCardTimes[wrongCardCount] = wrongTime;

      saveWrongCard(id, wrongTime);

      wrongCardCount++;

      wrongCount++;

      saveData();
    }

    lcd.print("WRONG CARD");

    lcd.setCursor(0,1);
    lcd.print(id);

    beep(5);
  }

  delay(1000);

  lcd.clear();
  lcd.print("Scan your card");

  rfid.PICC_HaltA();
}