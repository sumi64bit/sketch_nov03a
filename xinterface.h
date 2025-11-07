#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include "xmath.h"
#include <unordered_map>

#include "ebrima8pt7b.h"
#include <din1451alt_G10pt7b.h>

Arduino_GFX *GFX = nullptr;
TAMC_GT911 *TS = nullptr;

class TouchEvent{
    int hold_threshold = 500; //milliseconds
    int last_touch_time = 0;
    
    const int RELEASE_EXFRAMES = 2;
    int current_release_frame = 0;

    Vector2 last_touch_coordinates = {0, 0};
    public:
        enum Type{
          NONE,
          PRESS,
          RELEASE,
          HOLD,
          SWIPE
        } type;
        Vector2 touchPos(){
            return Vector2{TS->points[0].x, TS->points[0].y};
        }
        bool isPressed(int x, int y, int width, int height){
            if(type == PRESS || type == HOLD){
                Vector2 touch_coordinates = Vector2{TS->points[0].x, TS->points[0].y};
                if(touch_coordinates.x >= x && touch_coordinates.y >= y &&
                   touch_coordinates.x <= x + width && touch_coordinates.y <= y + height){
                    return true;
                }
            }
            return false;
        }
        void process(){
            if(TS->isTouched){
                if(!type == PRESS){
                    current_release_frame = 0;
                    Serial.println("TouchEvent: PRESS");
                    type = PRESS;
                    last_touch_time = millis();
                    last_touch_coordinates = Vector2{TS->points[0].x, TS->points[0].y};
                }else{
                    Vector2 touch_coordinates = Vector2{TS->points[0].x, TS->points[0].y};
                    if(touch_coordinates.x != last_touch_coordinates.x || touch_coordinates.y != last_touch_coordinates.y){
                        type = SWIPE;
                        Serial.println("TouchEvent: SWIPE");
                    }else if (type != SWIPE){
                        if(millis() - last_touch_time > hold_threshold){
                            if(type != HOLD){
                                type = HOLD;
                                Serial.println("TouchEvent: HOLD");
                            }
                        }
                    } 
                }
            }else{
                if(type == PRESS || type == HOLD){
                    type = RELEASE;
                    Serial.println("TouchEvent: RELEASE");
                }else{
                    if(current_release_frame < RELEASE_EXFRAMES){
                        current_release_frame++;
                    }else{
                        Serial.println("TouchEvent: NONE");
                        type = NONE;
                    }
                }
            }
        }
};

TouchEvent TEvent;


struct xelement{
  virtual void Process() = 0;
  virtual ~xelement() {}
};

struct Color{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  Color(unsigned char _r = 255, unsigned char _g = 255, unsigned char _b = 255){
    r = _r;
    g = _g;
    b = _b;
  }
  uint16_t hex(){
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
};

bool operator==(const Color& c1, const Color& c2){
    return (c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b);
}

Color lerpColor(Color col_1, Color col_2, float t = 0.5){
  Color temp_col = col_1;
  temp_col.r = std::lerp(temp_col.r, col_2.r, t);
  temp_col.g = std::lerp(temp_col.g, col_2.g, t);
  temp_col.b = std::lerp(temp_col.b, col_2.b, t);
  return temp_col;
}

struct Label : public xelement{
  String txt;
  int x;
  int y;
  Color col = Color(169, 169, 169);
  int font_size;

  Label(int _x, int _y, String _txt, int _size, Color _col = Color(255, 255, 255)){
    x = _x;
    y = _y;
    txt = _txt;
    font_size = _size;
    col = _col;
  }
  void Process(){
    Draw();
  }
  void Draw(){
    GFX->setFont(&ebrima8pt7b);
    GFX->setTextColor(col.hex()); 
    GFX->setCursor(x, y);
    GFX->setTextSize(font_size);
    GFX->println(txt);
  }
};

struct xButton : public xelement{
    int x, y;
    int width, height;
    String txt;
    Color button_color = Color(100, 100, 100);
    Color button_pressed_color = Color(150, 150, 150);
    int padding = 1;
    bool is_pressed = false;

    Label label = Label(0, 0, "Button", 1, Color(0, 0, 0));

    xButton(int _x = 0, int _y = 0, String _txt = "Button",
            int _b_width = 80, int _b_height = 40){
        x = _x;
        y = _y;
        txt = _txt;
        width = _b_width;
        height = _b_height;
        label.x = x + (width/2) - ((txt.length()*6)/2);
        label.y = y + (height/2) + 4;
        label.txt = txt;
    }
    void Process(){
        Draw();
    }
    void Draw(){
        GFX->fillRoundRect(x+padding, y, width-padding, height, 3, (is_pressed ? button_pressed_color : button_color).hex());
        label.Draw();
    }
    bool checkPressed(){
        TEvent.process();
        if(TEvent.type == TouchEvent::PRESS && !is_pressed && TEvent.isPressed(x, y, width, height)){
            Serial.println("Button pressed: " + txt);
            is_pressed = true;
            return true;
        }
        if(TEvent.type == TouchEvent::RELEASE){
            is_pressed = false;
        }
        return false;
    }
};

struct xPowerButton : public xelement{
    bool On = false;
    int x, y;
    int b_width, b_height;
    int padding;
    Color button_color = Color(24, 24, 24);
    Color pressed_color = Color(64, 64, 64);
    Color on_color = Color(0, 0, 255);
    Color off_color = Color(24, 24, 24);

    String label_txt = "On";
    Label label = Label(0, 0, label_txt, 1, Color(0, 0, 0));

    xPowerButton(int _x = 0, int _y = 0, int _padding = 0,
                 int _b_width = 80, int _b_height = 40){
        padding = _padding;
        x = _x;
        y = _y;
        b_width = _b_width;
        b_height = _b_height;

        label.x = x + (b_width/2) - ((label_txt.length()*6)/2);
        label.y = y + (b_height/2) + 4;
    }
    void Process(){
        Draw();
        checkPressed();
    }
    void Draw(){
        GFX->fillRoundRect(x+padding, y+padding, b_width-(padding*2), b_height-(padding*2), 3, button_color.hex());
        label.txt = (On ? "On" : "Off");
        label.col = (On ? Color(255, 255, 255) : Color(0, 0, 0));
        label.Draw();
    }
    void checkPressed(){
        TEvent.process();
        if(TEvent.type == TouchEvent::RELEASE && button_color == pressed_color){
            Serial.println("Power button pressed");
            On = !On;
            button_color = (On ? on_color : off_color);
            return;
        }
        if(TEvent.type == TouchEvent::NONE || TEvent.type == TouchEvent::SWIPE){
            button_color = (On ? on_color : off_color);
            return;
        }
        if (TEvent.isPressed(x, y, b_width, b_height)){
            if(TEvent.type == TouchEvent::PRESS || TEvent.type == TouchEvent::HOLD){
                button_color = pressed_color;
            }else{
                button_color = (On ? on_color : off_color);
            }
        }
    }
};

struct voltageToggleButton : public xelement{
    //rgba(51, 51, 51, 1);
    int toggled = 0;//0 = off, 1 = low voltage, 2 = high voltage
    int x, y;
    int width, height;
    int padding;
    Color normal_color = Color(24, 24, 24);
    Color pressed_color = Color(255, 220, 40);

    int toggled_rect_x = 0;

    Label label_first = Label(0, 0, "Test", 1, Color(0, 0, 0));
    Label label_second = Label(0, 0, "Test2", 1, Color(0, 0, 0));
    Label label_third = Label(0, 0, "Test3", 1, Color(0, 0, 0));

    voltageToggleButton(int is_toggled = 0,
               int _x = 0, int _y = 0, int _padding = 0,
               int _width = 240, int _height = 50, 
               Color _normal_col = Color(112, 112, 112), 
               Color _pressed_col = Color(255, 220, 40)){
        padding = _padding;
        x = _x;
        y = _y;
        width = _width;
        height = _height;
        toggled = is_toggled;
        normal_color = _normal_col;
        pressed_color = _pressed_col;
        label_first.x = x + (width/6) - 18;
        label_first.y = y + (height/2) + 4;
        label_second.x = x + (width/2) - 18;
        label_second.y = y + (height/2) + 4;
        label_third.x = x + ((width/6)*5) - 18;
        label_third.y = y + (height/2) + 4;

        toggled_rect_x = x + padding;
    }
    void Process(){
        Draw();
        animatedToggle();
        if(TS->isTouched){
            int ar_size = sizeof(TS->touches);
            int touch_x = TS->points[ar_size-1].x;
            int touch_y = TS->points[ar_size-1].y;
            if(touch_x>=x && touch_y>=y && touch_x <= x+width && touch_y <= y+height){
                if(touch_x <= x + (width/3)){
                    toggled = 0; //low voltage
                }else if(touch_x <= x + (width/3)*2){
                    toggled = 1; //high voltage
                }else{
                    toggled = 2; //off
                }
            }
        }
    }
    void Draw(){
        //Draw Background
        GFX->fillRoundRect(x, y, width, height, 3, normal_color.hex());
        //Draw First toggle button
        GFX->fillRoundRect(x+padding, y+padding, width/3, height-(padding*2), 3, (toggled==0 ? normal_color : normal_color).hex()); 
        //Draw Second toggle button
        GFX->fillRoundRect(x+padding+(width/3), y+padding, width/3, height-(padding*2), 3, (toggled==1 ? normal_color : normal_color).hex());
        //Draw Third toggle button
        GFX->fillRoundRect(x+padding+((width/3)*2), y+padding, width/3-2, height-(padding*2), 3, (toggled==2 ? normal_color : normal_color).hex());
        //Draw Borders 
        //GFX->drawRoundRect(x-padding, y-padding, width+(padding*2), height+(padding*2), 3, BLACK);
        //Draw animated toggled rect
        GFX->fillRoundRect(toggled_rect_x, y+padding, (width/3)-(padding*2), height-(padding*2), 3, pressed_color.hex());
        label_first.Process();
        label_second.Process();
        label_third.Process();
    }
    void animatedToggle(){
        int target_x;
        if(toggled == 0){
            target_x = x + padding;
        }else if(toggled == 1){
            target_x = x + padding + (width/3);
        }else{
            target_x = x + padding + ((width/3)*2);
        }
        toggled_rect_x = std::lerp(toggled_rect_x, target_x, 1);
    }
};
struct xPage{
    bool visible = true;
    enum Type{
        MAIN,
        SETTINGS,
        INFO
    } type;
    std::vector<std::shared_ptr<xelement>> elements;
    void AddElement(std::shared_ptr<xelement> element){
        elements.push_back(element);
    }
    void Process(){
        if(!visible) return;
        for(auto& element : elements){
            element->Process();
        }
    }
};
std::unordered_map<std::string, std::shared_ptr<xPage>> xPages;
