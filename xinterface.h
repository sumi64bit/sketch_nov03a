#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include "xmath.h"

Arduino_GFX *GFX = nullptr;
TAMC_GT911 *TS = nullptr;

class TouchEvent{
    int hold_threshold = 500; //milliseconds
    int last_touch_time = 0;
    Vector2 last_touch_coordinates = {0, 0};
    public:
        enum Type{
          NONE,
          PRESS,
          RELEASE,
          HOLD,
          SWIPE
        } type;
        void process(){
            if(TS->isTouched){
                if(!type == PRESS){
                    type = PRESS;
                    last_touch_time = millis();
                    last_touch_coordinates = Vector2{TS->points[0].x, TS->points[0].y};
                }else{
                    Vector2 touch_coordinates = Vector2{TS->points[0].x, TS->points[0].y};
                    if(touch_coordinates.x != last_touch_coordinates.x || touch_coordinates.y != last_touch_coordinates.y){
                        type = SWIPE;
                    }else if (type != SWIPE){
                        if(millis() - last_touch_time > hold_threshold){
                            type = HOLD;
                        }
                    } 
                }
            }else{
                if(type == PRESS || type == HOLD){
                    type = RELEASE;
                }else{
                    type = NONE;
                }
            }
        }
};

struct xelement{
  virtual void Draw() = 0;
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

Color lerpColor(Color col_1, Color col_2){
  Color temp_col = col_1;
  temp_col.r = std::lerp(temp_col.r, col_2.r, 0.5);
  temp_col.g = std::lerp(temp_col.g, col_2.g, 0.5);
  temp_col.b = std::lerp(temp_col.b, col_2.b, 0.5);
  return temp_col;
}

struct xdualToggleButton : public xelement{
    bool toggled = false;
    int x, y;
    int width, height;
    int padding;
    Color noraml_color = Color(112, 112, 112);
    Color pressed_color = Color(255, 220, 40);
    xdualToggleButton(bool is_toggled = false,
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
        //Draw ON toggle button
        GFX->fillRoundRect(x+padding, y+padding, width/2-(padding*2), height-(padding*2), 3, (toggled ? pressed_color : noraml_color).hex());
        GFX->setCursor(x + width/2, y + height/4);       
        GFX->setTextSize(2);
        GFX->setFont();
        GFX->setTextColor(BLACK);
        GFX->println("ON");  
        //Draw OFF toggle button
        GFX->fillRoundRect(x+padding+(width/2), y+padding, width/2+(padding*4), height-(padding*2), 3, (!toggled ? pressed_color : noraml_color).hex());
        GFX->setCursor(x + width/2, y + height + height/4 + (padding*2));       
        GFX->setTextColor(BLACK);
        GFX->println("OFF");
    }
};

