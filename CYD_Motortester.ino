
// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>
#include <Preferences.h>
#include <vector>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Bitbang.h>

#include <TFT_eSPI.h>

#include <UI.h>


struct ConstructButton
{
  String name;
  void (*callback)();
  bool isToggle;
  bool state;
};

struct RealButton
{
  TFT_eSPI_Button btn;
  void (*callback)();
  bool isToggle;
};
struct Point {
  int x;
  int y;
};


#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define SENSOR_PIN 35
#define IN2 22
#define PPO 20

#define MAIN_MENU std::vector<ConstructButton>{{"StartTest", StartRPMCount,false,false},{"Settings",OpenSettings, false, false}}
#define CANCEL_MENU std::vector<ConstructButton>{{"CancelTest", CancelTest,false,false}}
#define SETTINGS_MENU std::vector<ConstructButton>{{"Back", GetBackToMainMenu, false, false},{"doKick",setKickDo,true, doKick}}

Preferences prefs;
bool doKick;

volatile unsigned long pulseCount = 0;
volatile unsigned long startAt = -1;
std::vector<int> ppm = {};

volatile bool testFinished = false;
TaskHandle_t CoreOneTask = NULL;

SemaphoreHandle_t ppmMutex;
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);
TFT_eSPI tft = TFT_eSPI();

std::vector<UIComponent*> menuButtons;
Style btn = {TFT_WHITE,TFT_YELLOW,TFT_BLACK,TFT_BLUE};

void (*DoEveryFrame)();

//=================== Functions for UI ===================
void drawMenu(const std::vector<ConstructButton>& names) {
  // Clear screen
  tft.fillScreen(TFT_BLUE);

  // Top and bottom bars
  tft.fillRect(0, 0, TFT_HEIGHT, 30, TFT_YELLOW);
  tft.fillRect(0, TFT_WIDTH - 30, TFT_HEIGHT, 30, TFT_YELLOW);

  menuButtons.clear();

  int y = 60;

  for (ConstructButton name : names) {
    if(name.isToggle){
      Toggle* butn = new Toggle((int)TFT_HEIGHT/2,y,200,50,name.name,btn,name.callback,tft,name.state);
      menuButtons.push_back(butn);
    }else{
      Button* butn = new Button((int)TFT_HEIGHT/2,y,200,50,name.name,btn,name.callback,tft);
      menuButtons.push_back(butn);
    }

    y += 70;
  }
}


void getTouchedButton() {
  TouchPoint p = ts.getTouch();

  for (uint8_t b = 0; b < menuButtons.size(); ++b) {
    if ((p.zRaw > 0) && menuButtons[b]->btn.contains(p.x, p.y)) {
      menuButtons[b]->btn.press(true);
    } else {
      menuButtons[b]->btn.press(false);
    }
    if(menuButtons[b]->btn.justPressed()){
      menuButtons[b]->clicked();
      break;
    }
    else if(menuButtons[b]->btn.justReleased()){
      menuButtons[b]->released();
      break;
    }
  }
}


void drawStatusText(String message, int x, int y, uint8_t size, uint16_t color) {
  tft.setFreeFont(NULL);       // NULL removes the custom font and restores the default font
  tft.setTextSize(size);       // 1 is small (8px), 2 is medium (16px), 3 is large (24px)
  tft.setTextColor(color);     
  tft.drawString(message.c_str(), x, y); 
  tft.setFreeFont(&FreeMono18pt7b);
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


void EndMenu(){
  tft.fillScreen(TFT_BLUE);
  menuButtons.clear();
  menuButtons.push_back(new Button((int)TFT_HEIGHT / 2,25,200,50,(String)"Exit",btn,GetBackToMainMenu,tft));
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

  drawStatusText("Max RPM: " + String(maxRPM), 10, 60,1 ,TFT_WHITE);
  drawStatusText("Start PWM: " + pwmString, 180, 60, 1,TFT_WHITE);
  DrawGraph(ppm, *std::max_element(ppm.begin(), ppm.end()), 10, 80, 300, 160);
}
//=================== Motor functions ===================

void setup() {
  Serial.begin(115200);
  DoEveryFrame = getTouchedButton;
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
  drawMenu(MAIN_MENU);
  prefs.begin("settings",false);
  doKick = prefs.getBool("doKick",false);
  
}

void TestFinished(){
  getTouchedButton();
  if (testFinished) {
    testFinished = false; // Reset the trigger immediately
    EndMenu();            // Draw the menu cleanly without cross-core crashes
    DoEveryFrame = getTouchedButton;
  }
}

void loop() {
  DoEveryFrame();
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
  SetupMX(IN2);
  for(int i = 0;i<=256;){
    unsigned long millisP = millis()-lastMillis;
    if(doKick){
      forwardMX(IN2,250);
      Serial.println("doingKick");
    }
    if(millisP >= 250){
      noInterrupts();
      unsigned long snapshotPulses = pulseCount;
      pulseCount = 0;
      interrupts();
      float calculatedPPM = (snapshotPulses * (60000/millisP))/PPO;
      if (xSemaphoreTake(ppmMutex, portMAX_DELAY)){
        ppm.push_back(calculatedPPM);
        if((startAt == -1)&&(calculatedPPM > 0)){
          startAt = i;
        }
        xSemaphoreGive(ppmMutex);
      }
      lastMillis = millis();
      ++i;
    }
    forwardMX(IN2,i);
    if(testFinished){
      forwardMX(IN2,0);
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  testFinished = true;
  detachInterrupt(digitalPinToInterrupt(SENSOR_PIN));
  vTaskDelete(NULL);
}

//=================== Functions for this menu ===============
//Start Test
void StartRPMCount(){
  delay(500);
  ppm.clear();
  drawMenu(CANCEL_MENU);
  startAt = -1;
  DoEveryFrame = TestFinished;
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handlePulse, RISING);
  xTaskCreatePinnedToCore(
    CounterTask,   // Function to implement the task
    "PulseTask",        // Name of the task
    4096,               // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    &CoreOneTask,               // Task handle
    0                   // Core ID (0)
  );

}
//Settings
void OpenSettings(){
  menuButtons.clear();
  DoEveryFrame = getTouchedButton;
  drawMenu(SETTINGS_MENU);
}
//=================== Functions for this menu ===================
//CancelTest
void CancelTest(){
  forwardMX(IN2, 0); 
  detachInterrupt(digitalPinToInterrupt(SENSOR_PIN));
  // 3. Delete the running thread if it exists
  if (CoreOneTask != NULL) {
    vTaskDelete(CoreOneTask);
    CoreOneTask = NULL; // Reset handle tracking
  }
  // 4. Force the UI to show the final menu metrics screen
  testFinished = true;
}
//=================== Functions for this menu ===================
//Back
void GetBackToMainMenu(){
  drawMenu(MAIN_MENU);
  delay(500);
}
//doKick
void setKickDo(){
  doKick = !doKick;
  prefs.putBool("doKick", doKick);
}




