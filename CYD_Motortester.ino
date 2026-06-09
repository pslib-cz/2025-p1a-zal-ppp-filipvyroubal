
// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>
#include <Preferences.h>
#include <vector>
#include <unordered_map>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Bitbang.h>

#include <TFT_eSPI.h>

#include <UI.h>

#include <motorController.h>


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

#define SENSOR_PIN 27
#define IN2 22
#define PPO 20

#define MAIN_MENU std::vector<ConstructButton>{{"StartTest", StartRPMCount,false,false},{"Settings",OpenSettings, false, false}}
#define CANCEL_MENU std::vector<ConstructButton>{{"CancelTest", CancelTest,false,false}}
#define SETTINGS_MENU std::vector<ConstructButton>{{"Back", GetBackToMainMenu, false, false},{"doKick",setKickDo,true, doKick},{"stallProtection",setStallProtection,true,stallProtection}}

const unsigned int STILL_STALL_PWM = 80;

Preferences prefs;
bool doKick;
bool stallProtection;





MotorTest* tester;






XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);
TFT_eSPI tft = TFT_eSPI();

std::vector<UIComponent*> menuButtons;
std::unordered_map<std::string, UIComponent*> otherOnscreen;
const Style btn = {TFT_WHITE,TFT_YELLOW,TFT_BLACK,TFT_BLUE};
const Style pb = {TFT_BLACK,TFT_DARKGREY,TFT_BLACK,TFT_GREEN};

void (*DoEveryFrame)();

//=================== Functions for UI ===================
void drawMenu(const std::vector<ConstructButton>& names) {
  // Clear screen
  tft.fillScreen(TFT_BLUE);

  // Top and bottom bars
  tft.fillRect(0, 0, TFT_HEIGHT, 30, TFT_YELLOW);
  tft.fillRect(0, TFT_WIDTH - 30, TFT_HEIGHT, 30, TFT_YELLOW);
  for (UIComponent* btnPtr : menuButtons) {
    delete btnPtr;
  }
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
  int spacingX = (float)graphW / (float)(data.size() - 1);

  // Calculate the first point's Y position 
  // (Inverts the point so higher values go UP on the screen)
  int firstY = graphY + graphH - ((data[0] * graphH) / maxY);
  firstY = constrain(firstY, graphY, graphY + graphH - 1);
  Point LastPoint = {graphX, firstY};

  for (size_t i = 1; i < data.size(); ++i) {
    int nextY = graphY + graphH - ((data[i] * graphH) / maxY);
    nextY = constrain(nextY, graphY, graphY + graphH - 1);
    
    Point NowPoint = {graphX + (int)(i * spacingX), nextY};
    
    // Draw the graph line segment
    tft.drawLine(LastPoint.x, LastPoint.y, NowPoint.x, NowPoint.y, TFT_WHITE);
    
    LastPoint = NowPoint;
  }
}


void EndMenu(){
  tft.fillScreen(TFT_BLUE);
  for (UIComponent* btnPtr : menuButtons) {
    delete btnPtr;
  }
  menuButtons.clear();
  menuButtons.push_back(new Button((int)TFT_HEIGHT / 2,25,200,50,(String)"Exit",btn,GetBackToMainMenu,tft));
  int maxRPM = 0;
  String pwmString = "N/A";

  // Protect data reading from cross-core corruption


  maxRPM = tester->getMaxRPM();
  int startAt = tester->getStartAt();
  if (startAt != -1) {
    pwmString = String(startAt);
  }
  


  drawStatusText("Max RPM: " + String(maxRPM), 10, 60,1 ,TFT_WHITE);
  drawStatusText("Start PWM: " + pwmString, 180, 60, 1,TFT_WHITE);
  DrawGraph(tester->getGraphData(), maxRPM, 10, 80, 300, 160);
}


void errMenu(){
  tft.fillScreen(TFT_BLUE);
  for (UIComponent* btnPtr : menuButtons) {
    delete btnPtr;
  }
  menuButtons.clear();
  menuButtons.push_back(new Button(TFT_HEIGHT/2,25,200,50,(String)"Exit",btn,GetBackToMainMenu,tft));
  drawStatusText("Error occured test has not finished",0, 60, 4, TFT_BLACK);
}

void TestFinished(){
  getTouchedButton();
  updateData();
  if (tester->Finished()) {
    if(tester->Error()){
      errMenu();
    }
    else{
      EndMenu();            // Draw the menu cleanly without cross-core crashes
    }
    DoEveryFrame = getTouchedButton;
  }
}

void updateData(){
  static float lastPercentage = -1.0f;
  static int lastRPM = -1;
  float percentage = tester->getPercentage();
  if (abs(percentage - lastPercentage) > 0.005f) { 
    static_cast<ProgressBar*>(otherOnscreen["testPercentage"])->setProgress(percentage);
    lastPercentage = percentage;
  }
  static_cast<ProgressBar*>(otherOnscreen["testPercentage"])->setProgress(percentage);

  int currentRPM = tester->getLatestRPM();
  // Only redraw text if the RPM number actually changed
  if (currentRPM != lastRPM) {
    static_cast<KeyValue*>(otherOnscreen["actualRPM"])->changeValue(currentRPM);
    lastRPM = currentRPM;
  }

  
}


void setup() {
  Serial.begin(115200);
  DoEveryFrame = getTouchedButton;
  
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
  
  prefs.begin("settings",false);
  doKick = prefs.getBool("doKick",false);
  stallProtection = prefs.getBool("stallProtection",false);
  prefs.end();
  tester = new MotorTest(IN2,SENSOR_PIN,PPO,doKick,stallProtection,STILL_STALL_PWM);
  drawMenu(MAIN_MENU);
}

void loop() {
  DoEveryFrame();
}


//=================== Functions for this menu ===============
//Start Test
void StartRPMCount(){
  delay(500);
  drawMenu(CANCEL_MENU);
  for (auto& pair : otherOnscreen) {
    delete pair.second;
  }
  otherOnscreen.clear();
  otherOnscreen["testPercentage"] = new ProgressBar(0,menuButtons.back()->y+70,TFT_HEIGHT,50,pb,tft,"testPercentage");
  otherOnscreen["actualRPM"] = new KeyValue(0,0,TFT_HEIGHT, 30,"actualRPM",btn,tft,1,"actualRPM");
  tester->doKick = doKick;
  tester->stallProtection = stallProtection;
  tester->startTest();
  DoEveryFrame = TestFinished;
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
  tester->stopTest();
}
//=================== Functions for this menu ===================
//Back
void GetBackToMainMenu(){
  drawMenu(MAIN_MENU);
  delay(500);
}
//doKick
void setKickDo(){
  prefs.begin("settings",false);
  doKick = !doKick;
  prefs.putBool("doKick", doKick);
  prefs.end();
}
//stallProtection
void setStallProtection(){
  prefs.begin("settings",false);
  stallProtection = !stallProtection;
  prefs.putBool("stallProtection",stallProtection);
  prefs.end();
}




