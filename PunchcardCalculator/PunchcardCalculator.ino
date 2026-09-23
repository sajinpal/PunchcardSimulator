// ===================== FIRST ARDUINO MEGA =====================
// Keypad + P10 Punch Card + GLCD + Serial to 2nd Mega

#include <Keypad.h>
#include <SPI.h>
#include <DMD2.h>
#include <openGLCD.h>
#include <fonts/Arial14.h>

// ----------------- Keypad Setup -----------------
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'7','8','9','A'},
  {'4','5','6','B'},
  {'1','2','3','E'},
  {'0','0','.','.'}
};
byte rowPins[ROWS] = {44, 45, 46, 47};
byte colPins[COLS] = {48, 49, 50, 51};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// ----------------- P10 Punch Card Setup -----------------
SoftDMD dmd(2, 1);  // 64x16 display (2 P10 panels)
DMD_TextBox box(dmd, 0, 0, 64, 16);

const int rows = 10;
const int boxTopY = 3;
const int colSpacing = 1;
const int digitWidth = 4;
const int digitHeight = 10;

const byte punchDigits[10][10] = {
  {1,1,0,0,0,0,0,0,1,1},
  {0,0,0,0,0,0,0,0,1,1},
  {1,0,0,0,1,1,0,0,1,1},
  {1,0,0,0,1,1,0,0,1,1},
  {0,0,0,0,1,1,0,0,1,1},
  {1,1,0,0,1,1,0,0,1,1},
  {1,1,0,0,1,1,0,0,1,1},
  {1,0,0,0,1,1,0,0,0,0},
  {1,1,0,0,1,1,0,0,1,1},
  {1,1,0,0,1,1,0,0,1,1}
};

void drawDigit(int digit, int startX) {
  for (int y = 0; y < digitHeight; y++) {
    for (int x = 0; x < digitWidth; x++) {
      dmd.setPixel(startX + x, boxTopY + y, punchDigits[y][digit]);
    }
  }
}

void displayNumberLeft(int number) {
  int pos = 0;
  dmd.clearScreen();
  String numStr = String(number);
  for (int i = 0; i < numStr.length(); i++) {
    drawDigit(numStr.charAt(i) - '0', pos);
    pos += digitWidth + colSpacing;
  }
}

void displayNumberRight(int number) {
  int pos = 32;
  String numStr = String(number);
  for (int i = 0; i < numStr.length(); i++) {
    drawDigit(numStr.charAt(i) - '0', pos);
    pos += digitWidth + colSpacing;
  }
}

void displayPrompt(const char* msg) {
  GLCD.ClearScreen();
  GLCD.CursorTo(0, 0);
  GLCD.print(msg);
}

void displayPunchcardEquation(String a, String op, String b, String result) {
  dmd.clearScreen();
  int pos = 0;
  for (char c : a) {
    if (isdigit(c)) {
      drawDigit(c - '0', pos);
      pos += digitWidth + colSpacing;
    }
  }
  if (op.length() > 0) pos += digitWidth; // gap before operator
  for (char c : b) {
    if (isdigit(c)) {
      drawDigit(c - '0', pos);
      pos += digitWidth + colSpacing;
    }
  }
}

String getOpSymbol(char opCode) {
  switch (opCode) {
    case '+': return "+";
    case '-': return "-";
    case '*': return "*";
    case '/': return "/";
    default: return "";
  }
}

int a = 0, b = 0;
bool waitingForSecondInput = false;
bool inputsReady = false;
String liveInputA = "";
String liveInputB = "";
char currentOp = '+';

const int addButton = 39;
const int subButton = 40;
const int mulButton = 41;
const int divButton = 42;
const int execButton = 43;

void setup() {
  Serial.begin(9600);
  dmd.setBrightness(255);
  dmd.selectFont(SystemFont5x7);
  dmd.begin();
  GLCD.Init();
  GLCD.ClearScreen();
  displayPrompt("Enter A:");

  pinMode(addButton, INPUT_PULLUP);
  pinMode(subButton, INPUT_PULLUP);
  pinMode(mulButton, INPUT_PULLUP);
  pinMode(divButton, INPUT_PULLUP);
  pinMode(execButton, INPUT_PULLUP);
}

void loop() {
  char key = customKeypad.getKey();
  if (!inputsReady && key) {
    if (key == 'E') {
      if (!waitingForSecondInput && liveInputA.length() > 0) {
        a = liveInputA.toInt();
        displayNumberLeft(a);
        displayPrompt("Enter B:");
        waitingForSecondInput = true;
      } else if (waitingForSecondInput && liveInputB.length() > 0) {
        b = liveInputB.toInt();
        displayNumberRight(b);
        inputsReady = true;
        waitingForSecondInput = false;
        displayPrompt("Press operation btn");
      }
    } else if (key == 'A') {
      if (!waitingForSecondInput && liveInputA.length() > 0)
        liveInputA.remove(liveInputA.length() - 1);
      else if (waitingForSecondInput && liveInputB.length() > 0)
        liveInputB.remove(liveInputB.length() - 1);
    } else if (isdigit(key)) {
      if (!waitingForSecondInput && liveInputA.length() < 5)
        liveInputA += key;
      else if (waitingForSecondInput && liveInputB.length() < 5)
        liveInputB += key;
    }
    displayPunchcardEquation(liveInputA, getOpSymbol(currentOp), liveInputB, "");
  }

  if (inputsReady) {
    if (digitalRead(execButton) == LOW) {
      int result;
      switch (currentOp) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': result = b != 0 ? a / b : 0; break;
      }
      Serial.println(result);
      Serial1.println(result);
      displayPrompt(("Result: " + String(result)).c_str());
      displayPunchcardEquation(String(a), getOpSymbol(currentOp), String(b), String(result));

      while (digitalRead(execButton) == LOW);
      delay(100);
      resetSystem();
    }
  }

  if (digitalRead(addButton) == LOW) currentOp = '+';
  else if (digitalRead(subButton) == LOW) currentOp = '-';
  else if (digitalRead(mulButton) == LOW) currentOp = '*';
  else if (digitalRead(divButton) == LOW) currentOp = '/';
  delay(50);
}

void resetSystem() {
  a = 0; b = 0;
  liveInputA = "";
  liveInputB = "";
  inputsReady = false;
  waitingForSecondInput = false;
  currentOp = '+';
  dmd.clearScreen();
  GLCD.ClearScreen();
  displayPrompt("Enter A:");
}
