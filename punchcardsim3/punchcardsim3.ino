#include <SPI.h>
#include <DMD2.h>

SoftDMD dmd(2, 1);  // Two 32x16 panels = 64x16 display

const int rows = 10;         // Adjusted to match box height
const int colSpacing = 1;    // 1 pixel per digit
const int boxTopY = 3;       // Top Y inside box

// Updated 10-row punch card pattern for digits 0–9
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

// Draw one digit at a given column
void drawDigitPunch(int digit, int xOffset) {
  if (digit < 0 || digit > 9) return;
  for (int y = 0; y < rows; y++) {
    dmd.setPixel(xOffset, boxTopY + y, punchDigits[digit][y]);
  }
}

// Draw multiple digits in a limited-width box
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

void setup() {
  dmd.begin();
  dmd.setBrightness(100);
  dmd.clearScreen();

  // Draw 20x10 boxes
  //dmd.drawBox(2, 3, 21, 12);   // Left box
  //dmd.drawBox(34, 3, 53, 12);  // Right box

  const char* leftDigits  = "123";
  const char* rightDigits = "265";

  // X positions inside boxes (1 px padding)
  int leftBoxX = 13;
  int rightBoxX = 45;

  // Width available inside each box
  int boxContentWidth = 20;  // 20px box minus 1px margin each side

  drawPunchNumber(leftDigits, leftBoxX, boxContentWidth);
  drawPunchNumber(rightDigits, rightBoxX, boxContentWidth);
}

void loop() {
  // Static display
}
