 //topbar
 gfx->setFont(&din1451alt_G10pt7b);
  gfx->setCursor(60, 220);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(1);
  gfx->fillRect(0, 0, 480, 30, topbar);
  
  gfx->setTextColor(BLACK);
  gfx->setCursor(10, 20);
  gfx->print("4wire");
  gfx->setTextColor(BLACK);
  gfx->setCursor(240, 20);
  gfx->print("Temp: 35C");
  gfx->setCursor(120, 20);
  gfx->print("Pref:Volt");
  gfx->setCursor(425, 20);
  gfx->setFont();
  gfx->print("V1.0.0");
  gfx->setFont(&din1451alt_G15pt7b);
 

  gfx->setTextSize(1);
  gfx->setTextColor(text);
  gfx->setFont(&din1451alt_G25pt7b);
  gfx->setCursor(50, 80);
     if (volts1 < 10.0){
  gfx->printf("%.4f ",volts1);
     }
       else {
          gfx->printf("%.3f ",volts1);
       }
  gfx->setFont(&din1451alt_G15pt7b);
  gfx->setCursor(230, 80);
  gfx->print("V");

  
  gfx->setFont(&din1451alt_G25pt7b);
     if (volts < 10.0){
       gfx->setCursor(50, 130);
       gfx->printf("%.4f ",volts);
       }
       else {
       gfx->printf("%.3f ",volts);
       }
  gfx->setFont(&din1451alt_G15pt7b);
  gfx->setCursor(230, 130);
  gfx->print("A");

  
  gfx->setFont(&din1451alt_G15pt7b);
  gfx->setCursor(105, 170);
  gfx->printf("%.4f ",volts*volts1);
  gfx->setFont(&din1451alt_G15pt7b);
   gfx->setCursor(229, 170);
  gfx->print("w");

   
   gfx->setTextColor(WHITE);
 // gfx->setTextSize(1);
  gfx->setFont(&din1451alt_G10pt7b);
  gfx->setCursor(300, 80);
  gfx->print("OVP:         33.000 V");
  gfx->setCursor(300, 110);
  gfx->print("OCP:        11.000 A");
  gfx->setCursor(300, 140);
  gfx->print("On  Delay:  0.000 s");
  gfx->setCursor(300, 170);
  gfx->print("off  Delay:  0.000 s");
  gfx->setFont(&din1451alt_G15pt7b);
  
 
  gfx->setFont(&din1451alt_G10pt7b);
  gfx->fillRect(43,194,87,27,topbar);
  gfx->drawRect(163,194,87,27,topbar);
  gfx->setTextColor(BLACK);
 // gfx->setTextSize(1);
  gfx->setCursor(59,214); gfx->print("32.000");
  gfx->setTextColor(colText);
  gfx->setCursor(140,213); gfx->print("V");
  gfx->setTextColor(colText);
  gfx->setCursor(179,213);gfx->print("10.000");
  gfx->setCursor(262,213); gfx->print("A");
  gfx->setCursor(10,213); gfx->print("Set"); 
   
