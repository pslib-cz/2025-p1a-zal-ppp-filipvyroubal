#ifndef motorController_H
#define motorController_H

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


class MotorTest {
    private:
        const unsigned int motorPin;
        const unsigned int sensorPin;
        const unsigned int ppo;

        TaskHandle_t taskHandle = NULL;
        SemaphoreHandle_t mutex;

        std::vector<int> ppm;
        float percentage = 0.0f;
        int startAt = -1;
        bool isFinished = false;
        bool hasError = false;

        static void taskSub(void* pvParameters);
        void runTest();
    public:
        bool doKick;
        bool stallProtection;
        const unsigned int STILL_STALL_PWM;

        MotorTest(unsigned int motorPin, unsigned int sensorPin, unsigned int ppo, bool doKick, bool stallProtection, const unsigned int STILL_STALL_PWM);
        ~MotorTest();

        void startTest();
        void stopTest();
        void feedPulse();

        bool Finished();
        bool Error();
        float getPercentage();
        int getLatestRPM();
        int getMaxRPM();
        int getStartAt();
        std::vector<int> getGraphData();
};




#endif