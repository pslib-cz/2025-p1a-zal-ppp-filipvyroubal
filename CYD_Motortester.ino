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
#define IN2 35
#define PPO 20
// ----------------------------
volatile unsigned long pulseCount = 0;
volatile unsigned long startAt = -1;
volatile bool testFinished = false;
std::vector<int> ppm = {};
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
struct Point {
  int x;
  int y;
};
TFT_eSPI_Button key[6];
std::vector<RealButton> menuButtons;

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
  tft.fillScreen(TFT_BLUE);
  tft.setFreeFont(&FreeMono18pt7b);
  //SetupMX(IN2);
  StartRPMCount();
  //drawMenu({{"OK",StartRPMCount},{"Cancel",StartRPMCount}});
  
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
  getTouchedButton(menuButtons);
  if (testFinished) {
    testFinished = false; // Reset the trigger immediately
    EndMenu();            // Draw the menu cleanly without cross-core crashes
  }
}
void drawStatusText(String message, int x, int y, uint8_t size, uint16_t color) {
  tft.setFreeFont(NULL);       // NULL removes the custom font and restores the default font
  tft.setTextSize(size);       // 1 is small (8px), 2 is medium (16px), 3 is large (24px)
  tft.setTextColor(color);     
  tft.drawString(message.c_str(), x, y); 
}
void SetupMX(uint8_t pin){
  //ledcAttach(pin,5000, 8);
  ledcAttach(pin, 5000, 8);

}
void forwardMX(uint8_t pin,int pwm){
  ledcWrite(pin, pwm);
}
void IRAM_ATTR handlePulse(){
  ++pulseCount;
}
void CounterTask(void * pvParameters){
  unsigned long lastMillis = millis();
  for(int i = 0;i<=256;){
    if(millis()-lastMillis >= 100){
      noInterrupts();
      unsigned long snapshotPulses = pulseCount;
      pulseCount = 0;
      interrupts();
      float calculatedPPM = (snapshotPulses * 120.0)/PPO;
      if (xSemaphoreTake(ppmMutex, portMAX_DELAY)){
        ppm.push_back(calculatedPPM);
        if((startAt == NULL)&&(calculatedPPM > 0)){
          startAt = i;
        }
        xSemaphoreGive(ppmMutex);
      }
      lastMillis = millis();
      ++i;
      forwardMX(IN2,i);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  testFinished = true;
  detachInterrupt(digitalPinToInterrupt(SENSOR_PIN));
  vTaskDelete(NULL);
}
void StartRPMCount(){
  tft.fillScreen(TFT_BLUE);
  ppm.clear();
  menuButtons.clear();
  startAt = -1;
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handlePulse, RISING);
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
void EndMenu(){
  menuButtons.clear();
  TFT_eSPI_Button btn;
  btn.initButton(
    &tft,
    TFT_HEIGHT / 2,   // center X
    25,               // center Y
    200,             // width
    50,              // height
    TFT_WHITE,       // outline
    TFT_YELLOW,      // fill
    TFT_BLACK,       // text
    (char*)"Exit",    // label
    1                // text size
  );

  btn.drawButton(false);
  menuButtons.push_back({btn,StartRPMCount});
  int maxRPM = 0;
  String pwmString = "N/A";

  // Protect data reading from cross-core corruption
  if (xSemaphoreTake(ppmMutex, portMAX_DELAY)) {
    if (!ppm.empty()) {
      maxRPM = *std::max_element(ppm.begin(), ppm.end());
    }
    if (startAt != -1) {
      pwmString = String(startAt);
    }
    xSemaphoreGive(ppmMutex);
  }

  drawStatusText("Max RPM: " + String(maxRPM), 10, 60,9 ,TFT_WHITE);
  drawStatusText("Start PWM: " + pwmString, 180, 60, 9,TFT_WHITE);
  DrawGraph(ppm, 200, 10, 80, 300, 160);
}
void DrawGraph(std::vector<int> data, int maxY, int graphX, int graphY, int graphW, int graphH) {
  if (data.size() < 2) return; 
  if (maxY <= 0) maxY = 1;

  // Draw a black background bounding box for the graph area
  tft.fillRect(graphX, graphY, graphW, graphH, TFT_BLACK);

  // Calculate horizontal spacing between points
  int spacingX = graphW / (data.size() - 1);

  // Calculate the first point's Y position 
  // (Inverts the point so higher values go UP on the screen)
  int firstY = graphY + graphH - ((data[0] * graphH) / maxY);
  firstY = constrain(firstY, graphY, graphY + graphH - 1);
  Point LastPoint = {graphX, firstY};

  for (size_t i = 1; i < data.size(); ++i) {
    int nextY = graphY + graphH - ((data[i] * graphH) / maxY);
    nextY = constrain(nextY, graphY, graphY + graphH - 1);
    
    Point NowPoint = {LastPoint.x + spacingX, nextY};
    
    // Draw the graph line segment
    tft.drawLine(LastPoint.x, LastPoint.y, NowPoint.x, NowPoint.y, TFT_WHITE);
    
    LastPoint = NowPoint;
  }
}