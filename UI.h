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
    String id;

    UIComponent(int x, int y, int w, int h, String id) : x(x), y(y), width(w), height(h), id(id) {}
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
        
        Button(int x, int y, int width, int height, String label, Style style, void (*callback)(), TFT_eSPI &tft) : UIComponent(x,y,width,height, label){
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

    Toggle(int x, int y, int width, int height, String label, Style style, void (*callback)(), TFT_eSPI &tft, bool state) : UIComponent(x,y,width,height, label){
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

class ProgressBar : public UIComponent {
    public:
    TFT_eSPI &tft;
    Style style;
    ProgressBar(int x, int y, int width, int height, Style style, TFT_eSPI &tft, String id) : UIComponent(x,y,width,height, id), tft(tft){
        this->style = style;
        tft.fillRect(x,y,width,height,style.outline);
        tft.fillRect(x-10,y-10,width-10,height-10,style.fill);
    }
    void clicked() override{}
    void released() override{}
    void setProgress(float percentage){
        int w = percentage*(this->width-20);
        this->tft.fillRect(x-20,y-20,w,height - 20,this->style.activeFill);
    }
};

class KeyValue : public UIComponent {
    public:
        TFT_eSPI &tft;
        Style style;
        String label;
        int textSize;
        KeyValue(int x, int y, int width, int height, String label, Style style, TFT_eSPI &tft, int textSize, String id) : UIComponent(x,y,width,height, id), tft(tft){
            this->label = label;
            this->textSize = textSize;
            this->style = style;
            drawString(label);
        }
        void changeValue(int value){
            drawString(label+": "+String(value));
        }
        void drawString(String text){
            this->tft.setFreeFont(NULL);       // NULL removes the custom font and restores the default font
            this->tft.setTextSize(this->textSize);       // 1 is small (8px), 2 is medium (16px), 3 is large (24px)
            this->tft.setTextColor(this->style.text);     
            this->tft.drawString(text.c_str(), this->x, this->y); 
            this->tft.setFreeFont(&FreeMono18pt7b);
        }
        void clicked() override {}
        void released() override {}
};

#endif