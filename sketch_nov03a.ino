#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

#include <Wire.h>
#include "xinterface.h"

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
      hl_color = lerpColor(hl_color, c_button, 0.5);
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

struct fpsCounter{
  unsigned long lastTime = 0;
  int frameCount = 0;
  float fps = 0.0;

  void update(){
    frameCount++;
    unsigned long currentTime = millis();
    if(currentTime - lastTime >= 1000){
      fps = frameCount / ((currentTime - lastTime) / 1000.0);
      frameCount = 0;
      lastTime = currentTime;
    }
  }

  void draw(int x, int y){
    gfx->fillRoundRect(x, y, 80, 24, 5, BLACK);
    gfx->setCursor(x+2, y+12);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->print("FPS: ");
    gfx->println(fps);
  }
} fps;

fpsCounter FPSCounter;

class uiElements{
  public:
    voltageToggleButton dtb = voltageToggleButton(false, 0, 232, 1, 240, 40);
    xButton settings_button = xButton(240, 232, "Settings");
    xButton home_button = xButton(240, 232, "Home");
    xButton ex_button = xButton(320, 232, "EX");
    xPowerButton pb = xPowerButton(400, 232);

    uiElements(){}
    void init() {
      xPages["main"]->elements.push_back(std::make_unique<xButton>(settings_button));
      xPages["settings"]->elements.push_back(std::make_unique<xButton>(home_button));
      xPages["settings"]->visible = false;

      xElements.push_back(std::make_unique<voltageToggleButton>(dtb));
      xElements.push_back(std::make_unique<xPowerButton>(pb));
      //xElements.push_back(std::make_unique<xButton>(settings_button));
      xElements.push_back(std::make_unique<xButton>(ex_button));
    }
};

uiElements ui;

void setup() 
{
  GFX = gfx;
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

  xPages["main"] = std::make_shared<xPage>();
  xPages["settings"] = std::make_shared<xPage>();

  Label titleLabel = Label(220, 130, "Main page", 1, Color(255, 255, 255));
  xPages["main"]->elements.push_back(std::make_shared<Label>(titleLabel));

  Label settingsLabel = Label(200, 130, "Settings page", 1, Color(255, 255, 255));
  xPages["settings"]->elements.push_back(std::make_shared<Label>(settingsLabel));

  //set backlight control pin
  pinMode(GFX_BL, OUTPUT);
  ledcAttach(GFX_BL, freq, resolution);
  ui.init();

  gfx->fillRect(0, 240, 480, 32, button);
  int x = 5;
  
}

void loop() 
{ 
  //rgba(29, 29, 29, 1);
  gfx->fillScreen(Color(0, 0, 0).hex()); 
  int16_t cvc = ads.readADC_SingleEnded(0);
  float volts = ads.computeVolts(cvc);
  int value5 =map(cvc, 0, 32767, 0 ,30);

  int16_t cvc1 = ads.readADC_SingleEnded(1);
  float volts1 = ads.computeVolts(cvc1) * 10;
  int value51 =map(cvc1, 0, 32767, 0 ,30);
  

  //gfx->fillScreen(bg);
  ts.read();
  TEvent.process();
  /*
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
 */
  for (auto& _e : xElements) {
    _e->Process();
  }
  
  xPages["main"]->Process();
  xPages["settings"]->Process();
  if(ui.settings_button.checkPressed()){
    xPages["main"]->visible = false;
    xPages["settings"]->visible = true;
  }
  if(ui.home_button.checkPressed()){
    xPages["settings"]->visible = false;
    xPages["main"]->visible = true;
  }
  FPSCounter.update();
  FPSCounter.draw(10, 10);
  gfx->flush();
  ledcWrite(GFX_BL, 255);
}
