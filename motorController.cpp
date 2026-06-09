#include "MotorController.h"
#include <algorithm>

static volatile unsigned long globalPulseCount = 0;
void IRAM_ATTR handleMotorPulseISR() {
    globalPulseCount++;
}


MotorTest::MotorTest(unsigned int motorPin, unsigned int sensorPin, unsigned int ppo, bool doKick, bool stallProtection, const unsigned int STILL_STALL_PWM)
    : motorPin(motorPin), sensorPin(sensorPin), ppo(ppo), doKick(doKick), stallProtection(stallProtection), STILL_STALL_PWM(STILL_STALL_PWM)
{
    this->mutex = xSemaphoreCreateMutex();   
    ledcAttach(motorPin, 5000, 8); 
    pinMode(sensorPin, INPUT_PULLUP);
}


MotorTest::~MotorTest() {
    stopTest();
    vSemaphoreDelete(this->mutex);
}

void MotorTest::runTest(){
    unsigned long lastMillis = millis();
    if(doKick){
        ledcWrite(motorPin,250);
        Serial.println("doingKick");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    for(int i = 0;i < 256;){
        unsigned long millisP = millis()-lastMillis;
        
        if(millisP >= 250){

            noInterrupts();
            unsigned long snapshotPulses = globalPulseCount;
            globalPulseCount = 0;
            interrupts();

            float calculatedPPM = (snapshotPulses * (60000/millisP))/ppo;
            if (xSemaphoreTake(mutex, portMAX_DELAY)){

                ppm.push_back(calculatedPPM);

                if((startAt == -1)&&(calculatedPPM > 0)){
                    startAt = i;
                }

                percentage = (float)i/256.0f;

                if (stallProtection) {
                    
                    if ((i >= STILL_STALL_PWM) && (startAt == -1)) {      
                        isFinished = true;
                        hasError = true;
                        xSemaphoreGive(mutex);
                        break;
                    }
                    
                    else if (startAt != -1 && ppm.size() >= 5) {
                        bool allZeros = true;
                        int s = ppm.size();
                        
                        for (int k = s - 5; k < s;  k++) {
                            if (ppm[k] > 0) {
                                allZeros = false; 
                                
                                break;
                            }
                        }
                        
                        if (allZeros) {
                            Serial.println("Stall Protection: Motor se po startu zastavil!");
                            isFinished = true;
                            hasError = true;
                            xSemaphoreGive(mutex);
                            break;
                            
                        }
                    }
                }
                
                xSemaphoreGive(mutex);
            }
            lastMillis = millis();
            ++i;
            ledcWrite(motorPin,i);
            
        }
            
        if(isFinished){
            ledcWrite(motorPin,0);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    isFinished = true;

    ledcWrite(motorPin,0);

    detachInterrupt(digitalPinToInterrupt(sensorPin));

    taskHandle = NULL;
    
    vTaskDelete(NULL);
}
void MotorTest::taskSub(void* pvParameters){
    MotorTest* instance = static_cast<MotorTest*>(pvParameters);
    
    instance->runTest();
}

void MotorTest::startTest(){
    ppm.clear();
    percentage = 0.0f;
    startAt = -1;
    isFinished = false;
    hasError = false;
    globalPulseCount = 0;
    
    attachInterrupt(digitalPinToInterrupt(sensorPin), handleMotorPulseISR, RISING);

    // Vytvoření FreeRTOS úkolu na Jádře 0
    xTaskCreatePinnedToCore(
        taskSub, // Ukazatel na statický wrapper
        "MotorTestTask",
        4096,
        this,                       // Předáme "sami sebe" (this) jako parametr
        1,
        &taskHandle,
        0
    );
}


void MotorTest::stopTest() {
    
    if (taskHandle == NULL) {
        detachInterrupt(digitalPinToInterrupt(sensorPin));
        ledcWrite(motorPin, 0);
        return;
    }

    
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        isFinished = true; 
        xSemaphoreGive(mutex);
    }

    
    
    int timeout = 20; 
    while (taskHandle != NULL && timeout > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
        timeout--;
    }

    
    
    if (taskHandle != NULL) {
        Serial.println("Varování: Úkol neodpověděl, mažu ho natvrdo.");
        vTaskDelete(taskHandle);
        taskHandle = NULL;
    }

    
    detachInterrupt(digitalPinToInterrupt(sensorPin));
    ledcWrite(motorPin, 0);
}


bool MotorTest::Finished() {
    bool temp = false;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        temp = isFinished; xSemaphoreGive(mutex); }
    return temp;
}

bool MotorTest::Error() {
    bool temp = false;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        temp = hasError; xSemaphoreGive(mutex); }
    return temp;
}

float MotorTest::getPercentage() {
    float temp = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        temp = percentage; xSemaphoreGive(mutex); }
    return temp;
}

int MotorTest::getLatestRPM() {
    int temp = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        if (!ppm.empty()) temp = ppm.back();
        xSemaphoreGive(mutex);
    }
    return temp;
}

int MotorTest::getMaxRPM() {
    int temp = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        if (!ppm.empty()) {
            temp = *std::max_element(ppm.begin(), ppm.end());
        }
        xSemaphoreGive(mutex);
    }
    return temp;
}

int MotorTest::getStartAt() {
    int temp = -1;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) { 
        temp = startAt; xSemaphoreGive(mutex); }
    return temp;
}

std::vector<int> MotorTest::getGraphData() {
    std::vector<int> temp;
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        temp = ppm; // Vytvoří bezpečnou kopii vektoru pro vykreslení
        xSemaphoreGive(mutex);
    }
    return temp;
}