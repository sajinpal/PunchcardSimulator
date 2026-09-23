#include <SPI.h>
#include <DMD2.h>

SoftDMD dmd(2, 1);  // 2 horizontal P10 panels = 64x16

const int rows = 12;       // Number of punch card rows
const int colWidth = 3;    // 2 for digit + 1 for spacing
const int startY = 2;      // Y offset from top

// Punch card patterns for digits 0–9
const byte punchDigits[10][rows] = {
  {1,1,0,0,0,0,0,0,0,0,0,0}, // 0
  {0,1,0,0,0,0,0,0,0,0,0,0}, // 1
  {0,0,1,0,0,0,0,0,0,0,0,0}, // 2
  {0,0,0,1,0,0,0,0,0,0,0,0}, // 3
  {0,0,0,0,1,0,0,0,0,0,0,0}, // 4
  {0,0,0,0,0,1,0,0,0,0,0,0}, // 5
  {0,0,0,0,0,0,1,0,0,0,0,0}, // 6
  {0,0,0,0,0,0,0,1,0,0,0,0}, // 7
  {0,0,0,0,0,0,0,0,1,0,0,0}, // 8
  {0,0,0,0,0,0,0,0,0,1,0,0}  // 9
};

// Function to draw a punch digit at a given x offset
void drawDigitPunch(int digit, int xOffset) {
  for (int r = 0; r < rows; r++) {
    byte punch = punchDigits[digit][r];
    dmd.setPixel(xOffset,     startY + r, punch);
    dmd.setPixel(xOffset + 1, startY + r, punch);
  }
}

void setup() {
  dmd.begin();
  dmd.setBrightness(100);
  dmd.clearScreen();

  // === Panel 1 (0–31): Show digits "1" and "2" ===
  drawDigitPunch(1, 4);          // digit 1 starts near left
  drawDigitPunch(2, 4 + colWidth);

  // === Panel 2 (32–63): Show digits "2" and "5" ===
  drawDigitPunch(2, 36);         // 36 ≈ 32 + margin
  drawDigitPunch(5, 36 + colWidth);
}

void loop() {
  // Static display – no animation
}
