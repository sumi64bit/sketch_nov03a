#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <cmath>
#include <string>
#include "xinterface.h"

#include <din1451alt_G25pt7b.h>
#include <din1451alt_G15pt7b.h>
#include <vector>
#include <FreeSansBold4pt7b.h>
#include <din1451alt_G10pt7b.h>
//#include "SPIFFS.h"

#define GFX_BL 1 //  backlight pin


// setting PWM properties
const int freq = 5000;
const int resolution = 8;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
Arduino_GFX *g = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
Arduino_GFX *gfx = new Arduino_Canvas(480 /* width */, 272 /* height */, g);

#define SDA_PIN1 8
#define SCL_PIN1 4

#define RST_PIN 38 
#define INT_PIN 3
#define SDA_PIN 17
#define SCL_PIN 18

#define TOUCH_WIDTH 480
#define TOUCH_HEIGHT 272

TAMC_GT911 ts(SDA_PIN1, SCL_PIN1, INT_PIN, RST_PIN, TOUCH_WIDTH, TOUCH_HEIGHT);

uint16_t bg = gfx->color565(30, 20, 5);
uint16_t topbar = gfx->color565(255, 210, 0);
uint16_t text = gfx->color565(255, 255, 255);
uint16_t button = gfx->color565(60, 60, 60);
uint16_t accent = gfx->color565(255, 255, 0);
uint16_t colSetBox = gfx->color565(52,52,0);
uint16_t colText   = gfx->color565(255,255,255);

TouchEvent TEvent;

struct Label{
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
  void draw(){
    gfx->setCursor(x, y);
    gfx->setTextSize(font_size);
    gfx->println(txt);
  }
};

//THEME COLORS
Color c_button = Color(50, 50, 50);
Color c_button_pressed = Color(128, 128, 128);

struct xbutton{
  int x;
  int y;
  char* txt;
  bool is_pressed;
  int b_width;
  int b_height;
  int padding;
  Color hl_color = c_button;
  
  xbutton(int _x, int _y, char* _txt, int _width = 80, int _height = 40, int _padding = 1){
    txt = _txt;
    x = _x;
    y = _y;
    b_width = _width;
    b_height = _height; 
    padding = _padding;
  }
  void pressed(){
    is_pressed = true;
    hl_color = c_button_pressed;
  }
  void process(){
    int center_x = x+(b_width/2)-((strlen(txt)*6)/2);
    int center_y = y+b_height/2 - 4; 
    gfx->setFont();
    gfx->setTextSize(1);
    gfx->fillRoundRect(x+padding, y+padding, b_width-(padding*2), b_height-(padding*2), 3, hl_color.hex());
    gfx->drawRoundRect(x+padding, y+padding, b_width-(padding*2), b_height-(padding*2), 3, BLACK);
    gfx->setCursor(center_x, center_y);       
    gfx->setTextColor(WHITE);      
    gfx->println(txt);  
    if(is_pressed){
      hl_color = lerpColor(hl_color, c_button);
    }
  }
};

struct toggleButton{
  bool toggled = false;
  int x, y;
  int width, height;
  int padding;
  Color on_col = Color(0, 255, 0);
  Color off_col = Color(255, 0, 0);
  char* onLabel;
  char* offLabel;

  bool is_released = false;
  
  toggleButton(bool is_toggled = false,
               int _x = 0, int _y = 0, int _padding = 0,
               int _width = 50, int _height = 50, 
               Color _on_col = Color(0, 255, 0), 
               Color _off_col = Color(255, 0, 0), 
               char* _on_label = "ON", char* _off_label = "OFF"){
    padding = _padding;
    x = _x;
    y = _y;
    width = _width;
    height = _height;
    toggled = is_toggled;
    on_col = _on_col;
    off_col = _off_col;
  }
  void Draw(){
    Color _ACTIVE_COLOR;
    if(toggled){
      _ACTIVE_COLOR = on_col;
    }else{
      _ACTIVE_COLOR = off_col;
    }
    gfx->fillRoundRect(x+padding, y+padding, width-(padding*2), height-(padding*2), 3, _ACTIVE_COLOR.hex());
  }
  void pressed(){
    if(is_released){
      toggled = !toggled;
      is_released = false;
    }
  }
  void released(){
    is_released = true;
  }
};

std::vector<xbutton> buttons;
std::vector<toggleButton> toggleButtons;



void drawLock(int x1 = 50, int y1 = 50, uint16_t color2 = BLACK) {
  // lock body (10x8 rectangle)
  gfx->fillRect(x1 + 1, y1 + 4, 10, 8, color2);

  // lock shackle (U shape)
  gfx->drawPixel(x1 + 3, y1 + 3, color2);
  gfx->drawPixel(x1 + 4, y1 + 2, color2);
  gfx->drawPixel(x1 + 5, y1 + 2, color2);
  gfx->drawPixel(x1+ 6, y1 + 2, color2);
  gfx->drawPixel(x1 + 7, y1 + 3, color2);

  gfx->drawPixel(x1 + 2, y1 + 4, color2); // left shackle vertical
  gfx->drawPixel(x1 + 9, y1 + 4, color2); // right shackle vertical

  // optional keyhole
  gfx->drawPixel(x1 + 5, y1 + 7, BLACK);
  gfx->drawPixel(x1 + 5, y1 + 8, BLACK);
}

void drawPowerButton(int x, int y, uint16_t color) {
  int radius = 25; // half of 50x50
  int centerX = x + radius;
  int centerY = y + radius;

  // Draw the outer circle (power ring)
  gfx->drawCircle(centerX, centerY, radius, color);

  // Draw a slightly smaller filled circle to make it look like a ring
  gfx->fillCircle(centerX, centerY, radius - 5, BLACK);

  // Draw the vertical line in the top center (the “power line”)
  gfx->drawFastVLine(centerX, centerY - radius + 5, 15, color);
}


std::vector<std::unique_ptr<xelement>> xElements;

struct testBox{
  int x=10, y=10;
  bool dragging = false;
  testBox(){}
  void draw(){
    Color active_color;
    if(TEvent.type == TouchEvent::PRESS){
      active_color = Color(0, 255, 0);
    }else if(TEvent.type == TouchEvent::HOLD){
      active_color = Color(255, 0, 0);
      x = TEvent.touchPos().x - 50;
      y = TEvent.touchPos().y - 50;
      dragging = true;
    }else if(TEvent.type == TouchEvent::SWIPE){
      active_color = Color(0, 0, 255);
      if(dragging){
        x = TEvent.touchPos().x - 50;
        y = TEvent.touchPos().y - 50;
      }
    }else{
      active_color = Color(100, 100, 100);
      dragging = false;
    }
    gfx->fillRect(x, y, 100, 100, active_color.hex());
  }
};

testBox tbox;
void setup() 
{
  Serial.begin(115200);
  Wire1.begin(SDA_PIN, SCL_PIN);
  Wire.begin(SDA_PIN1, SCL_PIN1);
  ts.begin();
  ts.setRotation(90);
  TS = &ts;
  gfx->begin();

  Serial.begin(9600);
  ads.setGain(GAIN_ONE);

  if (!ads.begin(0x48, &Wire1)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  //set backlight control pin
  pinMode(GFX_BL, OUTPUT);
  ledcAttach(GFX_BL, freq, resolution);


    gfx->fillRect(0, 240, 480, 32, button);
  int x = 5;
  xdualToggleButton dtb = xdualToggleButton(false, 79*0+2, 232, 1, 152, 40);
  xbutton bt1 = xbutton(79*2+2, 232, "Home");
  xbutton bt2 = xbutton(79*3+2, 232, "Settings");
  xbutton bt3 = xbutton(79*4+2, 232, "More");
  xbutton bt4 = xbutton(79*5+2, 232, "About");

  buttons.push_back(bt1);
  buttons.push_back(bt2);
  buttons.push_back(bt3);
  buttons.push_back(bt4);
  xElements.push_back(std::make_unique<xdualToggleButton>(dtb));
}

void loop() 
{ 
  GFX = gfx;
  
 
  int16_t cvc = ads.readADC_SingleEnded(0);
  float volts = ads.computeVolts(cvc);
  int value5 =map(cvc, 0, 32767, 0 ,30);

   int16_t cvc1 = ads.readADC_SingleEnded(1);
  float volts1 = ads.computeVolts(cvc1) * 10;
  int value51 =map(cvc1, 0, 32767, 0 ,30);

  
  gfx->fillScreen(bg);
  

  ts.read();
  TEvent.process();
  tbox.draw();
  Serial.println(TEvent.type);
  for(xbutton& b: buttons){
    b.process();
    if (ts.isTouched){
      int ar_size = sizeof(ts.touches);
      int touch_x = ts.points[ar_size-1].x;
      int touch_y = ts.points[ar_size-1].y;
      if(touch_x>=b.x && touch_y>=b.y && touch_x <= b.x+b.b_width && touch_y <= b.y+b.b_height){
        b.pressed();
      }
    }
 }
 for(toggleButton& b: toggleButtons){
  b.Draw();
    if(ts.isTouched){
      int ar_size = sizeof(ts.touches);
      int touch_x = ts.points[ar_size-1].x;
      int touch_y = ts.points[ar_size-1].y;
      if(touch_x>=b.x && touch_y>=b.y && touch_x <= b.x+b.width && touch_y <= b.y+b.height){
        b.pressed();
      }
    }else{
      if (!b.is_released){
          b.released();
        }
    }
 }
  for (auto& _e : xElements) {
    _e->Process();
  }
 
   gfx->flush();
  ledcWrite(GFX_BL, 255);
     drawLock(100, 100, gfx->color565(255, 255, 0));
  gfx->fillScreen(BLACK); 
}
