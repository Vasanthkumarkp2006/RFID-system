// -------- Segment Pins (a,b,c,d,e,f,g) ----------
int segPins[7] = {23, 22, 21, 19, 18, 5, 17};

// -------- Digit Pins --------
// Road 1
int d1_1 = 16;
int d1_2 = 4;

// Road 2
int d2_1 = 2;
int d2_2 = 15;

// Road 3
int d3_1 = 34;
int d3_2 = 35;

// -------- Traffic Light Pins --------
// Road 1
int R1 = 13;
int Y1 = 12;
int G1 = 14;

// Road 2
int R2 = 27;
int Y2 = 26;
int G2 = 25;

// Road 3  ✅ CHANGED (no conflict now)
int R3 = 33;   // Changed
int Y3 = 32;   // Changed
int G3 = 0;   // Changed

// Common Anode Patterns
byte numbers[10][7] = {
  {0,0,0,0,0,0,1}, //0
  {1,0,0,1,1,1,1}, //1
  {0,0,1,0,0,1,0}, //2
  {0,0,0,0,1,1,0}, //3
  {1,0,0,1,1,0,0}, //4
  {0,1,0,0,1,0,0}, //5
  {0,1,0,0,0,0,0}, //6
  {0,0,0,1,1,1,1}, //7
  {0,0,0,0,0,0,0}, //8
  {0,0,0,0,1,0,0}  //9
};

void setup() {

  for(int i=0;i<7;i++)
    pinMode(segPins[i], OUTPUT);

  pinMode(d1_1, OUTPUT);
  pinMode(d1_2, OUTPUT);
  pinMode(d2_1, OUTPUT);
  pinMode(d2_2, OUTPUT);
  pinMode(d3_1, OUTPUT);
  pinMode(d3_2, OUTPUT);

  pinMode(R1, OUTPUT); pinMode(Y1, OUTPUT); pinMode(G1, OUTPUT);
  pinMode(R2, OUTPUT); pinMode(Y2, OUTPUT); pinMode(G2, OUTPUT);
  pinMode(R3, OUTPUT); pinMode(Y3, OUTPUT); pinMode(G3, OUTPUT);
}

// ---------------- Display Function ----------------
void displayNumber(int num, int digit1, int digit2) {

  int tens = num / 10;
  int ones = num % 10;

  // Tens
  digitalWrite(digit1, LOW);
  digitalWrite(digit2, HIGH);
  for(int i=0;i<7;i++)
    digitalWrite(segPins[i], numbers[tens][i]);
  delay(5);

  // Ones
  digitalWrite(digit1, HIGH);
  digitalWrite(digit2, LOW);
  for(int i=0;i<7;i++)
    digitalWrite(segPins[i], numbers[ones][i]);
  delay(5);
}

// -------- Set All Roads Red --------
void allRed() {
  digitalWrite(R1, HIGH); digitalWrite(Y1, LOW); digitalWrite(G1, LOW);
  digitalWrite(R2, HIGH); digitalWrite(Y2, LOW); digitalWrite(G2, LOW);
  digitalWrite(R3, HIGH); digitalWrite(Y3, LOW); digitalWrite(G3, LOW);
}

// -------- Countdown Function --------
void countdown(int seconds, int digit1, int digit2, int R, int Y, int G, bool isGreen) {

  allRed();   // Other roads red

  if(isGreen) {
    digitalWrite(G, HIGH);
  } else {
    digitalWrite(Y, HIGH);
  }

  digitalWrite(R, LOW);

  for(int i = seconds; i >= 0; i--) {
    unsigned long start = millis();
    while(millis() - start < 1000) {
      displayNumber(i, digit1, digit2);
    }
  }

  digitalWrite(G, LOW);
  digitalWrite(Y, LOW);
}

void loop() {

  digitalWrite(Y1, HIGH);

  // -------- ROAD 1 --------
  countdown(10, d1_1, d1_2, R1, Y1, G1, true);   // Green 10
  countdown(3,  d1_1, d1_2, R1, Y1, G1, false);  // Yellow 3

  // -------- ROAD 2 --------
  countdown(5,  d2_1, d2_2, R2, Y2, G2, true);   // Green 5
  countdown(3,  d2_1, d2_2, R2, Y2, G2, false);  // Yellow 3

  // -------- ROAD 3 --------
  countdown(10, d3_1, d3_2, R3, Y3, G3, true);   // Green 10
  countdown(3,  d3_1, d3_2, R3, Y3, G3, false);  // Yellow 3
}
