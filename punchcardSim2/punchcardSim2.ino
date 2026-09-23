#include <SPI.h>
#include <DMD2.h>

SoftDMD dmd(2, 1);  // Two 32x16 panels = 64x16 total display

const int rows = 12;
const int boxTopY = 2;     // Y start of box (same as box drawing)
const int colSpacing = 1;  // 1 pixel per digit

// Punch card representation (1 column per digit)
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

// Draw one digit (single column) inside the box
void drawDigitPunch(int digit, int xOffset) {
  if (digit < 0 || digit > 9) return;

  for (int y = 0; y < rows; y++) {
    dmd.setPixel(xOffset, boxTopY + y, punchDigits[digit][y]);
  }
}

// Draw multiple digits inside a defined box
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

  // Draw two boxes
  dmd.drawBox(2, 2, 29, 13);   // Left box: panel 1
  dmd.drawBox(34, 2, 61, 13);  // Right box: panel 2

  // Content to display inside each box
  const char* leftDigits  = "12";
  const char* rightDigits = "25";

  // Compute box widths
  int leftBoxX = 3;                  // Inside-left of box
  int leftBoxW = 29 - 2 - 1;         // Width inside left box (26px)

  int rightBoxX = 35;                // Inside-right of box
  int rightBoxW = 61 - 34 - 1;       // Width inside right box (26px)

  // Draw punch digits inside boxes
  drawPunchNumber(leftDigits, leftBoxX, leftBoxW);
  drawPunchNumber(rightDigits, rightBoxX, rightBoxW);
}

void loop() {
  // Static display
}
