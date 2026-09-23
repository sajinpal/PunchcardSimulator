#include <SPI.h>
#define GLCD_TIMER_RATE_HZ 0

#include <DMD2.h>
#include <openGLCD.h>
// Include necessary font headers. lcdnums14x24 is expected to be implicitly 
// defined by openGLCD.h, but we keep explicit includes for common fonts.
#include "fonts/fixednums15x31.h" 
#include "fonts/Arial14.h" 
// We rely on the font 'lcdnums14x24' being correctly compiled 
// when the openGLCD library was installed.

#include <Keypad.h>
#include <avr/wdt.h>
#include <avr/io.h>

// Define the smaller font for numbers (approx. 24 pixels high)
#define NUMBER_FONT lcdnums12x16 

// === Keypad Setup (Unchanged) ===
const byte ROWS = 5;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  { '1','2','3','E' },
  { '#','$','%','@' }, 
  { '7','8','9','A' },
  { '4','5','6','B' },
  { '0','0','.','.' } 
};
byte rowPins[ROWS] = { 44, 45, 46, 47, 48 };
byte colPins[COLS] = { 49, 50, 51, 52 };
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// === Display Setup (Unchanged) ===
SoftDMD dmd(2, 1);
const int rows = 10;
const int boxTopY = 4;
const int colSpacing = 1;

// === Pin Definitions (Unchanged) ===
const int ADD_PIN = 40, SUB_PIN = 41, MUL_PIN = 39, DIV_PIN = 38, EXECUTE_PIN = 43, LED_PIN = 12;

// === Punchcard Data (Unchanged) ===
const byte punchDigits[10][rows] = {
  {1,0,0,0,0,0,0,0,0,0}, {0,1,0,0,0,0,0,0,0,0},
  {0,0,1,0,0,0,0,0,0,0}, {0,0,0,1,0,0,0,0,0,0},
  {0,0,0,0,1,0,0,0,0,0}, {0,0,0,0,0,1,0,0,0,0},
  {0,0,0,0,0,0,1,0,0,0}, {0,0,0,0,0,0,0,1,0,0},
  {0,0,0,0,0,0,0,0,1,0}, {0,0,0,0,0,0,0,0,0,1}
};

// === Operation Management (8-digit setup using 'long') ===
enum Operation { NONE, ADD, SUB, MUL, DIV };
Operation currentOp = NONE;
bool operationSelected = false, resultShown = false, inputsReady = false, waitingForSecondInput = false;
long long a = 0, b = 0; 
char liveInputA[12]; 
char liveInputB[12]; 

// === Timing & Watchdog (FIXED: Added lastDebounceTime) ===
unsigned long lastActivityTime = 0;
unsigned long lastDebounceTime = 0; // CORRECTED: Re-added this declaration
const unsigned long debounceDelay = 50;
const unsigned long INACTIVITY_TIMEOUT = 30000;

// === Utility (Unchanged) ===
void updateActivity() {
  lastActivityTime = millis();
}
bool isIdleTooLong() {
  return (millis() - lastActivityTime > INACTIVITY_TIMEOUT);
}

// ----------------------------------------------------------------------
// === Safe GLCD/DMD wrappers (Using NUMBER_FONT) ===
// ----------------------------------------------------------------------
void safeDmdClear() {
  wdt_reset();
  noInterrupts();       
  dmd.clearScreen();
  interrupts();
  wdt_reset();
  delayMicroseconds(100);
}
void safeClearGLCD() {
  wdt_reset();
  noInterrupts();
  GLCD.ClearScreen();
  interrupts();
  wdt_reset();
  delayMicroseconds(100);
}
void safeFillRect(int x, int y, int w, int h, int color) {
  wdt_reset();
  noInterrupts();
  GLCD.FillRect(x, y, w, h, color);
  interrupts();
  wdt_reset();
  delayMicroseconds(80);
}
void safeSelectFontAndPrint(int x, int y, const __FlashStringHelper* txt) {
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(Arial14);
  GLCD.CursorToXY(x, y);
  GLCD.print(txt);
  interrupts();
  wdt_reset();
  delayMicroseconds(20);
}
void safePrintLongLeft(int x, int y, long num) {
  char buf[12];
  sprintf(buf, "%ld", num); 
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(NUMBER_FONT);
  GLCD.CursorToXY(x, y);
  GLCD.print(buf);
  interrupts();
  wdt_reset();
}
void safePrintLongRight(int x, int y, long num) {
  char buf[12];
  sprintf(buf, "%ld", num); 
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(NUMBER_FONT);
  GLCD.CursorToXY(66, 22);
  GLCD.print(buf);
  interrupts();
  wdt_reset();
}
void safePrintStrLeft(int x, int y, const char* s) {
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(NUMBER_FONT);
  GLCD.CursorToXY(x, y);
  GLCD.print(s);
  interrupts();
  wdt_reset();
}
void safePrintStrRight(int x, int y, const char* s) {
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(NUMBER_FONT);
  GLCD.CursorToXY(x, y);
  GLCD.print(s);
  interrupts();
  wdt_reset();
}

// ----------------------------------------------------------------------
// === Punchcard Drawing (Sign Visualization) ===
// ----------------------------------------------------------------------
void drawDigitPunch(int digit, int xOffset) {
  for (int y = 0; y < rows; y++)
    dmd.setPixel(xOffset, boxTopY + y, punchDigits[digit][y]);
}
void drawPunchNumber(const char* number, int xStart, int maxWidth) {
  for (int i = 0; number[i] && (i * colSpacing) < maxWidth; i++) {
    // Only process digits for the punchcard pattern
    if (number[i] >= '0' && number[i] <= '9') drawDigitPunch(number[i] - '0', xStart + i * colSpacing);
  }
}
void drawOperatorSymbol(char op, int x) {
  switch (op) {
    case '+': dmd.setPixel(x, boxTopY+4, 1); dmd.setPixel(x+1, boxTopY+4, 1); dmd.setPixel(x+2, boxTopY+4, 1);
              dmd.setPixel(x+1, boxTopY+3, 1); dmd.setPixel(x+1, boxTopY+5, 1); break;
    case '-': dmd.setPixel(x, boxTopY+4, 1); dmd.setPixel(x+1, boxTopY+4, 1); dmd.setPixel(x+2, boxTopY+4, 1); break;
    case '*': dmd.setPixel(x, boxTopY+3, 1); dmd.setPixel(x+2, boxTopY+3, 1);
              dmd.setPixel(x+1, boxTopY+4, 1); dmd.setPixel(x, boxTopY+5, 1); dmd.setPixel(x+2, boxTopY+5, 1); break;
    case '/': dmd.setPixel(x+2, boxTopY+2, 1); dmd.setPixel(x+1, boxTopY+4, 1); dmd.setPixel(x, boxTopY+6, 1); break;
  }
}

// Shows the FINAL result on the DMD (MODIFIED to display sign)
void displayPunchcardEquation(const char* num1, char op, const char* num2, const char* result, int sign) {
  dmd.clearScreen();
  drawPunchNumber(num1, 8, 5);
  drawPunchNumber(num2, 8 + 5 * colSpacing, 5);
  drawOperatorSymbol(op, 13 + 10 * colSpacing);
  
  // Set the sign indicator on the punch card (Col 58 for positive, 59 for negative)
  // These columns are near the edge of the display (assuming 64 wide DMD)
  if (sign == 1) {
    // Positive: 19th column in a visual sense
    dmd.setPixel(58, boxTopY, 1); 
  } else if (sign == -1) {
    // Negative: 20th column in a visual sense
    dmd.setPixel(59, boxTopY, 1);
  }

  // Draw the result. 'result' should be sign-stripped if negative.
  if (result[0] != '\0') 
    drawPunchNumber(result, 40, 64 - 40);
  updateActivity(); 
}

void safeClearRegionDMD(int x, int y, int w, int h) {
  wdt_reset();
  dmd.drawFilledBox(x, y, x + w - 1, y + h - 1, GRAPHICS_OFF);
  wdt_reset();
}

// Shows the LIVE typing on the DMD (Unchanged)
void displayPunchcardLIVE(const char* num1, char op, const char* num2) {
  dmd.clearScreen();
  drawPunchNumber(num1, 8, 5);
  drawPunchNumber(num2, 8 + 5 * colSpacing, 5);
  if(op != ' ') { 
    drawOperatorSymbol(op, 13 + 10 * colSpacing);
  }
  updateActivity();
}

// ----------------------------------------------------------------------
// === GLCD Display (Using NUMBER_FONT) ===
// ----------------------------------------------------------------------
void displayPrompt(const __FlashStringHelper* prompt) {
  safeFillRect(0, 0, 128, 20, WHITE);
  safeSelectFontAndPrint(2, 4, prompt);
  updateActivity();
}

void displayNumberLeft(long num) {
  safeFillRect(0, 22, 64, 42, WHITE);
  safePrintLongLeft(2, 22, num);
  updateActivity();
}

void displayNumberRight(long num) {
  safeFillRect(64, 22, 64, 42, WHITE);
  wdt_reset();
  noInterrupts();
  GLCD.SelectFont(NUMBER_FONT);
  GLCD.CursorToXY(66, 22);
  safePrintLongRight(66, 22, num);
  interrupts();
  wdt_reset();
  updateActivity();
}

void displayLiveInputLeft(const char* num, bool backspace) {
  if (backspace) safeFillRect(0, 22, 64, 42, WHITE); 
  safePrintStrLeft(2, 22, num);
}
void displayLiveInputRight(const char* num, bool backspace) {
  if (backspace) safeFillRect(64, 22, 64, 42, WHITE); 
  safePrintStrRight(66, 22, num);
}

char getOpSymbol(Operation op) {
  switch (op) { case ADD: return '+'; case SUB: return '-'; case MUL: return '*'; case DIV: return '/'; default: return '?'; }
}

// === Transmit to Slave Arduino (Unchanged) ===
void transmitResult(long a, long b, Operation op, const char* resultStr) {
  char opChar = getOpSymbol(op);
  char msg[40]; 
  // resultStr retains the negative sign for numerical/serial output
  sprintf(msg, "%ld%c%ld=%s\n", a, opChar, b, resultStr);
  Serial1.print(msg);
}

// ----------------------------------------------------------------------
// === Operation Handling (Handle sign removal and "Not Defined") ===
// ----------------------------------------------------------------------
void handleOperation() {
  char resultStr[15] = {0};   // For GLCD/Serial (keeps sign if negative) or "Not Defined"
  char resultDMD[15] = {0};   // For Punchcard (sign removed if negative)
  char aStr[15] = {0}; 
  char bStr[15] = {0}; 
  int sign = 0; // 1: positive, -1: negative, 0: zero/error

  sprintf(aStr, "%ld", a);
  sprintf(bStr, "%ld", b);

  switch (currentOp) {
    case ADD: 
    case SUB: 
    case MUL: {
      long long res;
      if (currentOp == ADD) res = a + b;
      else if (currentOp == SUB) res = a - b;
      else res = a * b;
      
      sprintf(resultStr, "%ld", res);
      
      if (res > 0) sign = 1; else if (res < 0) sign = -1;
      
      // Handle negative sign removal for punchcard
      if (sign == -1) {
          // Skip the first character ('-')
          strcpy(resultDMD, &resultStr[1]); 
      } else {
          strcpy(resultDMD, resultStr);
      }
      break;
    }
    case DIV:
      if (b != 0) {
        double res = (double)a / (double)b; 
        dtostrf(res, 0, 8, resultStr); 
        
        if (res > 0) sign = 1; else if (res < 0) sign = -1;
        
        // Handle negative sign removal for punchcard (floating point)
        if (sign == -1) {
            // Skip the first character ('-')
            strcpy(resultDMD, &resultStr[1]);
        } else {
            strcpy(resultDMD, resultStr);
        }
      } else {
        // Division by zero
        strcpy(resultStr, "Not Defined"); // For GLCD/Serial
        strcpy(resultDMD, ""); // Empty punchcard result
        sign = 0;
      }
      break;
    default:
      strcpy(resultStr, "ERR");
      strcpy(resultDMD, ""); // Empty punchcard result
      sign = 0;
      break;
  }

  // Use resultDMD (no sign) for punchcard drawing
  displayPunchcardEquation(aStr, getOpSymbol(currentOp), bStr, resultDMD, sign);
  // Use resultStr (with sign or "Not Defined") for transmission
  transmitResult(a, b, currentOp, resultStr);
  
  // Display the final result on the GLCD 
  safeClearGLCD();
  safeSelectFontAndPrint(2, 4, F("Result:"));
  // Use resultStr (which contains "Not Defined" or the sign) for GLCD display
  safePrintStrLeft(2, 22, resultStr); 

  resultShown = true;
  digitalWrite(LED_PIN, HIGH);
  updateActivity();
}

// ----------------------------------------------------------------------
// === System Setup and Loop ===
// ----------------------------------------------------------------------
void resetSystem() {
  wdt_reset();
  currentOp = NONE;
  operationSelected = false;
  resultShown = false;
  inputsReady = false;
  waitingForSecondInput = false;
  a = b = 0;
  liveInputA[0] = '\0';
  liveInputB[0] = '\0'; 
  digitalWrite(LED_PIN, LOW);

  safeClearGLCD();
  safeDmdClear();

  displayPrompt(F("Enter Digit 1:"));
  updateActivity();

  Serial1.print("CLEAR\n");
  wdt_reset();
}

void setup() {
  wdt_disable();
  Serial1.begin(9600);

  pinMode(ADD_PIN, INPUT_PULLUP);
  pinMode(SUB_PIN, INPUT_PULLUP);
  pinMode(MUL_PIN, INPUT_PULLUP);
  pinMode(DIV_PIN, INPUT_PULLUP);
  pinMode(EXECUTE_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  GLCD.Init();

  dmd.begin();
  dmd.setBrightness(100);

  liveInputA[0] = '\0';
  liveInputB[0] = '\0';

  resetSystem();

  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();

  if (isIdleTooLong()) resetSystem();

  static bool prevExec = HIGH;
  bool execState = digitalRead(EXECUTE_PIN);

  // --- Handle Execute Button (Debounced) ---
  if (operationSelected && !resultShown && prevExec == HIGH && execState == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      handleOperation();
      lastDebounceTime = millis();
    }
  }
  prevExec = execState;

  // --- Handle Reset Condition (after result is shown) ---
  if (resultShown && execState == HIGH &&
      digitalRead(ADD_PIN) == HIGH && digitalRead(SUB_PIN) == HIGH &&
      digitalRead(MUL_PIN) == HIGH && digitalRead(DIV_PIN) == HIGH) {
    resetSystem();
  }

  // --- Handle Keypad Input ---
  char key = customKeypad.getKey();
  if (key && !resultShown) {
    updateActivity();

    if (key == 'A' || key == 'B') { // Backspace
      if (!waitingForSecondInput && strlen(liveInputA) > 0) {
        liveInputA[strlen(liveInputA) - 1] = '\0';
        displayLiveInputLeft(liveInputA, true);
        displayPunchcardLIVE(liveInputA, ' ', "");
      } else if (waitingForSecondInput && strlen(liveInputB) > 0) {
        liveInputB[strlen(liveInputB) - 1] = '\0';
        displayLiveInputRight(liveInputB, true);
        displayPunchcardLIVE(liveInputA, ' ', liveInputB);
      }
    }
    else if (key == 'E') { // Enter
      if (!waitingForSecondInput && strlen(liveInputA) > 0) {
        a = atol(liveInputA); 
        displayNumberLeft(a);
        displayPrompt(F("Enter Digit 2:"));
        waitingForSecondInput = true;
      } else if (waitingForSecondInput && strlen(liveInputB) > 0) {
        b = atol(liveInputB); 
        displayNumberRight(b);
        inputsReady = true;
        waitingForSecondInput = false;
        displayPrompt(F("Select Operation"));
      }
    }
    else if (key >= '0' && key <= '9') { // Digits (8-digit limit)
      if (!waitingForSecondInput && strlen(liveInputA) < 4) { 
        int len = strlen(liveInputA);
        liveInputA[len] = key;
        liveInputA[len + 1] = '\0';
        displayLiveInputLeft(liveInputA, false);
        displayPunchcardLIVE(liveInputA, ' ', "");
      }
      else if (waitingForSecondInput && strlen(liveInputB) < 4) { 
        int len = strlen(liveInputB);
        liveInputB[len] = key;
        liveInputB[len + 1] = '\0';
        displayLiveInputRight(liveInputB, false);
        displayPunchcardLIVE(liveInputA, ' ', liveInputB);
      }
    }
  }

  // --- Handle Operation Buttons (Debounced) ---
  if (inputsReady && !operationSelected) {
    if (millis() - lastDebounceTime > debounceDelay) {
      Operation newOp = NONE;
      if (digitalRead(ADD_PIN) == LOW) newOp = ADD;
      else if (digitalRead(SUB_PIN) == LOW) newOp = SUB;
      else if (digitalRead(MUL_PIN) == LOW) newOp = MUL;
      else if (digitalRead(DIV_PIN) == LOW) newOp = DIV;

      if (newOp != NONE) {
        currentOp = newOp;
        operationSelected = true;
        lastDebounceTime = millis();
        updateActivity();
        displayPunchcardLIVE(liveInputA, getOpSymbol(currentOp), liveInputB);
        displayPrompt(F("Press Execute"));
      }
    }
  }
}