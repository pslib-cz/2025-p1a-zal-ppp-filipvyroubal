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

#define SENSOR_PIN 22
// ----------------------------
volatile unsigned long pulseCount = 0;
volatile float ppm = 0.0;
SemaphoreHandle_t ppmMutex;
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

TFT_eSPI tft = TFT_eSPI();
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
TFT_eSPI_Button key[6];
std::vector<RealButton> menuButtons;

void IRAM_ATTR handlePulse(){
  ++pulseCount;
}
void CounterTask(void * pvParameters){
  unsigned long lastMillis = millis();
  for(;;){
    if(millis()-lastMillis >= 5000){
      noInterrupts();
      unsigned long snapshotPulses = pulseCount;
      pulseCount = 0;
      interrupts();
      float calculatedPPM = snapshotPulses * 12.0;
      if (xSemaphoreTake(ppmMutex, portMAX_DELAY)){
        ppm = calculatedPPM;
        xSemaphoreGive(ppmMutex);
      }
      lastMillis = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void StartRPMCount(){
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handlePulse, FALLING);
  xTaskCreatePinnedToCore(
    CounterTask,   // Function to implement the task
    "PulseTask",        // Name of the task
    4096,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL,               // Task handle
    0                   // Core ID (0)
  );

}
void setup() {
  Serial.begin(115200);
  
  ppmMutex = xSemaphoreCreateMutex();
  // Create the background task on Core 0
  
  // Start the SPI for the touch screen and init the TS library
  ts.begin();
  //ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); //This is the display in landscape

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeMono18pt7b);
  drawMenu({{"OK",StartRPMCount},{"Cancel",StartRPMCount}});
  
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
void getTouchedButton(std::vector<RealButton> &btns) {
  TouchPoint p = ts.getTouch();
  
  // 1. Map your touch coordinates (Adjustment may be needed based on calibration)
  // Most CYD displays need mapping from raw touch (approx 0-4000) to pixels (0-320)
  uint16_t x = map(p.x, 200, 3700, 0, 320);
  uint16_t y = map(p.y, 240, 3800, 0, 240);

  // 2. Corrected loop condition: b < btns.size()
  for (uint8_t b = 0; b < btns.size(); ++b) {
    if ((p.zRaw > 0) && btns[b].btn.contains(p.x, p.y)) {
      btns[b].btn.press(true);
      btns[b].btn.drawButton(true);
      //btns[b].callback();
    } else {
      btns[b].btn.press(false);
      btns[b].btn.drawButton(false);
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
  //getTouchedButton(menuButtons);
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
  float localPPM = 0.0;
  
  // Safely grab the latest PPM calculated by Core 0
  if (xSemaphoreTake(ppmMutex, int(10 / portTICK_PERIOD_MS))) {
    localPPM = ppm;
    xSemaphoreGive(ppmMutex);
  }
  
  // Update your CYD display here with the localPPM value
  // Example: tft.drawString("PPM: " + String(localPPM), 10, 10);
  
  Serial.print("Current PPM: ");
  Serial.println(localPPM);
  delay(50);
}