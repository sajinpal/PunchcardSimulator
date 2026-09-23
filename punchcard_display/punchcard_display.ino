#include <SPI.h>
#include <DMD2.h>

SoftDMD dmd(2, 1);  // Two 32x16 panels = 64x16

const int rows = 10;
const int boxTopY = 3;
const int colSpacing = 1;
String inputLine = "";

// Corrected punchDigits: One hole per digit at the correct row
const byte punchDigits[10][rows] = {
  {1,0,0,0,0,0,0,0,0,0}, // 0: hole at row 0
  {0,1,0,0,0,0,0,0,0,0}, // 1: hole at row 1
  {0,0,1,0,0,0,0,0,0,0}, // 2: hole at row 2
  {0,0,0,1,0,0,0,0,0,0}, // 3: hole at row 3
  {0,0,0,0,1,0,0,0,0,0}, // 4: hole at row 4
  {0,0,0,0,0,1,0,0,0,0}, // 5: hole at row 5
  {0,0,0,0,0,0,1,0,0,0}, // 6: hole at row 6
  {0,0,0,0,0,0,0,1,0,0}, // 7: hole at row 7
  {0,0,0,0,0,0,0,0,1,0}, // 8: hole at row 8
  {0,0,0,0,0,0,0,0,0,1}  // 9: hole at row 9
};

void drawDigitPunch(int digit, int xOffset) {
  if (digit < 0 || digit > 9) return;
  for (int y = 0; y < rows; y++)
    dmd.setPixel(xOffset, boxTopY + y, punchDigits[digit][y]);
}

// Rest of the code remains unchanged...

void drawPunchNumber(const char* number, int xStart, int maxWidth) {
  int i = 0;
  while (number[i] && (xStart + i * colSpacing) < (xStart + maxWidth)) {
    char ch = number[i];
    if (ch >= '0' && ch <= '9')
      drawDigitPunch(ch - '0', xStart + i * colSpacing);
    i++;
  }
}

void drawOperatorSymbol(char op, int x) {
  switch (op) {
    case '+':
      dmd.setPixel(x+1, boxTopY+3, 1);
      dmd.setPixel(x+1, boxTopY+4, 1);
      dmd.setPixel(x+1, boxTopY+5, 1);
      dmd.setPixel(x,   boxTopY+4, 1);
      dmd.setPixel(x+2, boxTopY+4, 1);
      break;
    case '-':
      dmd.setPixel(x,   boxTopY+4, 1);
      dmd.setPixel(x+1, boxTopY+4, 1);
      dmd.setPixel(x+2, boxTopY+4, 1);
      break;
    case '*':
      dmd.setPixel(x,   boxTopY+3, 1);
      dmd.setPixel(x+2, boxTopY+3, 1);
      dmd.setPixel(x+1, boxTopY+4, 1);
      dmd.setPixel(x,   boxTopY+5, 1);
      dmd.setPixel(x+2, boxTopY+5, 1);
      break;
    case '/':
      dmd.setPixel(x+2, boxTopY+2, 1);
      dmd.setPixel(x+1, boxTopY+4, 1);
      dmd.setPixel(x,   boxTopY+6, 1);
      break;
  }
}

void displayEquation(String num1, char op, String num2, String result) {
  dmd.clearScreen();
  int startX_input = 13;
  int startX_sum = 45;

  drawPunchNumber(num1.c_str(), startX_input + 0, 5);
  drawPunchNumber(num2.c_str(), startX_input + 5, 5);
  drawOperatorSymbol(op,        startX_input + 10);

  drawPunchNumber(result.c_str(), startX_sum, 64 - startX_sum);
}

String calculate(String num1, char op, String num2) {
  long a = num1.toInt();
  long b = num2.toInt();
  long result;

  if (op == '+') result = a + b;
  else if (op == '-') result = a - b;
  else if (op == '*') result = a * b;
  else if (op == '/') {
    if (b == 0) return "ERR";
    result = a / b;
  } else return "ERR";

  return String(result);
}

void parseAndDisplay(String input) {
  input.trim();
  int opIndex = input.indexOf('+');
  if (opIndex == -1) opIndex = input.indexOf('-');
  if (opIndex == -1) opIndex = input.indexOf('*');
  if (opIndex == -1) opIndex = input.indexOf('/');

  if (opIndex == -1 || opIndex == 0 || opIndex == input.length()-1) {
    displayEquation("0", '?', "0", "ERR");
    return;
  }

  String num1 = input.substring(0, opIndex);
  char op = input.charAt(opIndex);
  String num2 = input.substring(opIndex + 1);

  String result = calculate(num1, op, num2);
  displayEquation(num1, op, num2, result);
}

void setup() {
  Serial.begin(9600);
  dmd.begin();
  dmd.setBrightness(100);
  dmd.clearScreen();
  Serial.println("Punchcard Calculator Ready!");
  Serial.println("Enter expression like: 23+7 or 40/2");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        parseAndDisplay(inputLine);
        inputLine = "";
      }
    } else {
      inputLine += c;
    }
  }
}