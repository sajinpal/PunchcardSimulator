#include <SPI.h>
#include <DMD2.h>

SoftDMD dmd(2, 1);  // 2 panels = 64x16

const int rows = 10;
const int boxTopY = 3;
const int colSpacing = 1;

const byte punchDigits[10][rows] = {
  {1,1,0,0,0,0,0,0,0,0}, // 0
  {0,1,0,0,0,0,0,0,0,0}, // 1
  {0,0,1,0,0,0,0,0,0,0}, // 2
  {0,0,0,1,0,0,0,0,0,0}, // 3
  {0,0,0,0,1,0,0,0,0,0}, // 4
  {0,0,0,0,0,1,0,0,0,0}, // 5
  {0,0,0,0,0,0,1,0,0,0}, // 6
  {0,0,0,0,0,0,0,1,0,0}, // 7
  {0,0,0,0,0,0,0,0,1,0}, // 8
  {0,0,0,0,0,0,0,0,0,1}  // 9
};

void drawDigitPunch(int digit, int xOffset) {
  if (digit < 0 || digit > 9) return;
  for (int y = 0; y < rows; y++) {
    dmd.setPixel(xOffset, boxTopY + y, punchDigits[digit][y]);
  }
}

void drawPunchNumber(const char* number, int xStart, int maxWidth) {
  int i = 0;
  while (number[i] && (xStart + i * colSpacing) < (xStart + maxWidth)) {
    char ch = number[i];
    if (ch >= '0' && ch <= '9') {
      drawDigitPunch(ch - '0', xStart + i * colSpacing);
    }
    i++;
  }
}

void drawPlusSymbol(int x) {
  dmd.setPixel(x+1, boxTopY + 3, 1);
  dmd.setPixel(x+1, boxTopY + 4, 1);
  dmd.setPixel(x+1, boxTopY + 5, 1);
  dmd.setPixel(x,   boxTopY + 4, 1);
  dmd.setPixel(x+2, boxTopY + 4, 1);
}

void setup() {
  dmd.begin();
  dmd.setBrightness(100);
  dmd.clearScreen();

  // Inputs
  const char* num1 = "12";
  const char* num2 = "34";
  const char* num3 = "56";

  // Sum
  long sum = atol(num1) + atol(num2) + atol(num3);
  char sumStr[21];
  ltoa(sum, sumStr, 10);

  // Start positions
  int startX_input = 13;
  int startX_sum = 45;

  // Draw inputs on panel 1
  drawPunchNumber(num1, startX_input + 0, 5);
  drawPunchNumber(num2, startX_input + 5, 5);
  drawPunchNumber(num3, startX_input + 10, 5);
  drawPlusSymbol(startX_input + 15);  // "+" in 3px

  // Draw sum on panel 2 starting from pixel 45
  drawPunchNumber(sumStr, startX_sum, 64 - startX_sum);
}

void loop() {
  // Static display
}
