/*******************************************************************
    TFT_eSPI button example for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    Written by Claus Näveke
    Github: https://github.com/TheNitek
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>
#include <Preferences.h>
// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Bitbang.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046 Slim")
// https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library

#include <TFT_eSPI.h>
#include <vector>

// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
// https://github.com/Bodmer/TFT_eSPI


// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
// ----------------------------

XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

TFT_eSPI tft = TFT_eSPI();

TFT_eSPI_Button key[6];
std::vector<RealButton> menuButtons;
struct ConstructButton
{
  String name;
  void (*callback)();
};

struct RealButton
{
  TFT_eSPI_Button btn;
  void (*callback)();
};


void setup() {
  Serial.begin(115200);

  // Start the SPI for the touch screen and init the TS library
  ts.begin();
  //ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); //This is the display in landscape

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeMono18pt7b);
  drawMenu({"OK","Cancel"});
  
}
void drawMenu(const std::vector<ConstructButton>& names) {
  // Clear screen
  tft.fillScreen(TFT_BLUE);

  // Top and bottom bars
  tft.fillRect(0, 0, TFT_HEIGHT, 30, TFT_YELLOW);
  tft.fillRect(0, TFT_WIDTH - 30, TFT_HEIGHT, 30, TFT_YELLOW);

  menuButtons.clear();

  int y = 60;

  for (ConstructButton name : names) {
    TFT_eSPI_Button btn;

    btn.initButton(
      &tft,
      TFT_HEIGHT / 2,   // center X
      y,               // center Y
      200,             // width
      50,              // height
      TFT_WHITE,       // outline
      TFT_YELLOW,      // fill
      TFT_BLACK,       // text
      (char*)name.name.c_str(),    // label
      1                // text size
    );

    btn.drawButton(false);
    menuButtons.push_back({btn,name.callback});

    y += 70;
  }
}


void drawButtons() {
  uint16_t bWidth = TFT_HEIGHT/3;
  uint16_t bHeight = TFT_WIDTH/2;
  // Generate buttons with different size X deltas
  for (int i = 0; i < 6; i++) {
    key[i].initButton(&tft,
                      bWidth * (i%3) + bWidth/2,
                      bHeight * (i/3) + bHeight/2,
                      bWidth,
                      bHeight,
                      TFT_BLACK, // Outline
                      TFT_BLUE, // Fill
                      TFT_BLACK, // Text
                      "",
                      1);

    key[i].drawButton(false, String(i+1));
  }
}
// Added & to pass by reference so state changes persist
void getTouchedButton(std::vector<TFT_eSPI_Button> &btns) {
  TouchPoint p = ts.getTouch();
  
  // 1. Map your touch coordinates (Adjustment may be needed based on calibration)
  // Most CYD displays need mapping from raw touch (approx 0-4000) to pixels (0-320)
  uint16_t x = map(p.x, 200, 3700, 0, 320);
  uint16_t y = map(p.y, 240, 3800, 0, 240);

  // 2. Corrected loop condition: b < btns.size()
  for (uint8_t b = 0; b < btns.size(); ++b) {
    if ((p.zRaw > 0) && btns[b].contains(p.x, p.y)) {
      btns[b].press(true);
      btns[b].drawButton(true);
    } else {
      btns[b].press(false);
      btns[b].drawButton(false);
    }
  }
  /*
  for (uint8_t b = 0; b < btns.size(); ++b) {
    if (btns[b].justPressed()) {
      Serial.printf("Button %d pressed\n", b);
      btns[b].drawButton(true);
    }

    if (btns[b].justReleased()) {
      Serial.printf("Button %d released\n", b);
      btns[b].drawButton(false);
    }
  }*/
}
void loop() {
  getTouchedButton(menuButtons);
  /*
  TouchPoint p = ts.getTouch();
  // Adjust press state of each key appropriately
  for (uint8_t b = 0; b < 6; b++) {
    if ((p.zRaw > 0) && key[b].contains(p.x, p.y)) {
      key[b].press(true);  // tell the button it is pressed
    } else {
      key[b].press(false);  // tell the button it is NOT pressed
    }
  }

  // Check if any key has changed state
  for (uint8_t b = 0; b < 6; b++) {
    // If button was just pressed, redraw inverted button
    if (key[b].justPressed()) {
      Serial.printf("Button %d pressed\n", b);
      key[b].drawButton(true, String(b+1));
    }

    // If button was just released, redraw normal color button
    if (key[b].justReleased()) {
      Serial.printf("Button %d released\n", b);
      Serial.println("Button " + (String)b + " released");
      key[b].drawButton(false, String(b+1));
    }
  }
  delay(50);*/
}