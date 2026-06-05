#ifndef UI_H
#define UI_H

#include <SPI.h>
#include <Preferences.h>
#include <vector>
#include <XPT2046_Bitbang.h>
#include <TFT_eSPI.h>

struct Style {
    int outline;
    int fill;
    int text;
    int activeFill;
};

// This acts as our Interface
class UIComponent {
public:
    int x;
    int y;
    int width;
    int height;
    TFT_eSPI_Button btn;

    UIComponent(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    // Pure virtual functions (notice the = 0)
    // Every class inheriting from this MUST provide its own version of these
    virtual void clicked() = 0;
    virtual void released() = 0;
    // A virtual destructor is required for interfaces
    virtual ~UIComponent() {} 
};

class Button : public UIComponent {
    public:
        String name;
        void (*callback)();
        
        Button(int x, int y, int width, int height, String label, Style style, void (*callback)(), TFT_eSPI &tft) : UIComponent(x,y,width,height){
            this->name = label;
            this->callback = callback;
            this->btn.initButton(
            &tft,
            x,   // center X
            y,               // center Y
            width,             // width
            height,              // height
            style.outline,       // outline
            style.fill,      // fill
            style.text,       // text
            (char*)label.c_str(),    // label
            1                // text size
            );
            this->btn.drawButton(false);
        }
        void clicked() override{
            this->btn.drawButton(true);
            this->callback();
        }
        void released() override{
            this->btn.drawButton(false);
        }

};

class Toggle : public UIComponent {
    public:
    String name;
    void (*callback)();
    bool state;

    Toggle(int x, int y, int width, int height, String label, Style style, void (*callback)(), TFT_eSPI &tft, bool state) : UIComponent(x,y,width,height){
        this->name = label;
        this->callback = callback;
        this->btn.initButton(
        &tft,
        x,   // center X
        y,               // center Y
        width,             // width
        height,              // height
        style.outline,       // outline
        style.fill,      // fill
        style.text,       // text
        (char*)label.c_str(),    // label
        1                // text size
        );
        this->btn.drawButton(state);

        this->state = state;
        if(this->state){
            this->callback();
        }

    }
    void clicked() override{
        this->state = !this->state;
        this->btn.drawButton(this->state);
        this->callback();
    }
    void released() override{
    }
};

#endif