#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include "xmath.h"
#include <unordered_map>

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
    GFX->setFont(&din1451alt_G10pt7b);
    GFX->setTextColor(col.hex()); 
    GFX->setCursor(x, y);
    GFX->setTextSize(font_size);
    GFX->println(txt);
  }
};

struct xPowerButton : public xelement{
    bool On = false;
    int x, y;
    int b_width, b_height;
    int padding;
    Color button_color = Color(255, 102, 82);
    Color pressed_color = Color(255, 255, 255);

    Color on_color = Color(111, 255, 82);
    Color off_color = Color(255, 102, 82);

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
    bool toggled = false;
    int x, y;
    int width, height;
    int padding;
    Color noraml_color = Color(112, 112, 112);
    Color pressed_color = Color(255, 220, 40);
    xToggleButton(bool is_toggled = false,
               int _x = 0, int _y = 0, int _padding = 0,
               int _width = 50, int _height = 50, 
               Color _normal_col = Color(112, 112, 112), 
               Color _pressed_col = Color(255, 220, 40)){
        padding = _padding;
        x = _x;
        y = _y;
        width = _width;
        height = _height;
        toggled = is_toggled;
        noraml_color = _normal_col;
        pressed_color = _pressed_col;
    }
    void Process(){
        Draw();
        
        if(TS->isTouched){
            int ar_size = sizeof(TS->touches);
            int touch_x = TS->points[ar_size-1].x;
            int touch_y = TS->points[ar_size-1].y;
            if(touch_x>=x && touch_y>=y && touch_x <= x+width && touch_y <= y+height){
                if(touch_x <= x + width/2){
                    toggled = true;
                }else{
                    toggled = false;
                }
            }
        }
    }
    void Draw(){
        //Draw First toggle button
        GFX->fillRoundRect(x+padding, y+padding, width/3-(padding*2), height-(padding*2), 3, (toggled ? pressed_color : noraml_color).hex()); 
        //Draw Second toggle button
        GFX->fillRoundRect(x+padding+(width/3), y+padding, width/3+(padding*4), height-(padding*2), 3, (!toggled ? pressed_color : noraml_color).hex());
        //Draw Third toggle button
        GFX->fillRoundRect(x+padding+((width/3)*2), y+padding, width/3-(padding*2), height-(padding*2), 3, noraml_color.hex());
        //Draw Borders
        GFX->drawRoundRect(x-padding, y-padding, width+(padding*2), height+(padding*2), 3, BLACK);
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
        for(auto& element : elements){
            element->Process();
        }
    }
};
std::unordered_map<std::string, std::shared_ptr<xPage>> xPages;
