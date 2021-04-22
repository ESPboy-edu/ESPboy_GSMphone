#include <ESP8266WiFi.h>
#include "Adafruit_MCP23017.h"
#include "Adafruit_MCP4725.h"
#include "TFT_eSPI.h"
#include "ESPboy_LED.h"
#include "lib/glcdfont.c"
#include "lib/ESPboyLogo.h"
#include "ESPboyGSMlib.h"

#define LHSWAP(w)       (((w)>>8)|((w)<<8))
#define MCP23017address 0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)
#define MCP4725address  0x60 //DAC driving LCD backlit
#define LEDPIN          D4
#define SOUNDPIN        D3
#define CSTFTPIN        8 //CS MCP23017 PIN to TFT
#define VIBROPIN        9 //VIBRO MCP23017 PIN

#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff

#define GSM_RX      D6
#define GSM_TX      D8

#define MAX_CONSOLE_STRINGS 10
#define MAX_TYPING_CHARS 50
#define KEY_UNPRESSED_TIMEOUT 700
#define KEY_PRESSED_DELAY_TO_SEND 500
#define CURSOR_BLINKING_PERIOD 500
#define TFT_FADEOUT_DELAY 50000

Adafruit_MCP23017 mcp;
Adafruit_MCP4725 dac;
ESPboyLED myled;
TFT_eSPI tft = TFT_eSPI();
ESPboyGSM GSM(GSM_RX, GSM_TX);       // RX, TX


constexpr uint8_t keybOnscr[3][21] PROGMEM = {
"+1234567890ABCDEFGHI",
"JKLMNOPQRSTUVWXYZ_-=",
"?!@#$%&*()_[]\":;.,<E",
};

static String consolestrings[MAX_CONSOLE_STRINGS+1];
static uint16_t consolestringscolor[MAX_CONSOLE_STRINGS+1];
uint16_t line_buffer[46];
uint8_t keyState = 0;
int8_t selX = 0, selY = 0;

String typing = "";
uint32_t cursorBlinkMillis = 0;
uint8_t cursorTypeFlag = 0;
char cursorType[2]={220,'_'};
uint8_t sendFlag = 0;

int16_t lcdFadeBrightness;
uint32_t lcdFadeTimer;

int16_t GSMringFlag;
uint32_t GSMringTimer;
int16_t GSMsmsFlag;
uint32_t GSMsmsTimer;


String gsmResponse = ""; 


uint8_t getKeys() { return (~mcp.readGPIOAB() & 255); }


void drawConsole(String bfrstr, uint16_t color){
 for (uint8_t i=0; i<=(bfrstr.length()/21); i++)
    drawConsoleNow(bfrstr.substring(i*20),color);
}


void drawConsoleNow(String bfrstr, uint16_t color){
  for (int i=0; i<MAX_CONSOLE_STRINGS; i++) {
    consolestrings[i] = consolestrings[i+1];
    consolestringscolor[i] = consolestringscolor[i+1];
  }
  
  if (bfrstr.length()>20) bfrstr = bfrstr.substring(0,20);
  consolestrings[MAX_CONSOLE_STRINGS-1] = bfrstr;
  consolestringscolor[MAX_CONSOLE_STRINGS-1] = color;
  tft.fillRect(1,1,126,MAX_CONSOLE_STRINGS*8+1,TFT_BLACK);
  for (int i=0; i<MAX_CONSOLE_STRINGS; i++)
    printFast(4, i*8+2, consolestrings[i], consolestringscolor[i], TFT_BLACK);
}



void drawCharFast(uint16_t x, uint16_t y, uint8_t c, uint16_t color, uint16_t bg){
 uint16_t i, j, c16, line;
  for (i = 0; i < 5; ++i){
    line = pgm_read_byte(&font[c * 5 + i]);
    for (j = 0; j < 8; ++j){
      c16 = (line & 1) ? color : bg;
      line_buffer[j * 5 + i] = LHSWAP(c16);
      line >>= 1;
    }
  }
  tft.pushImage(x, y, 5, 8, line_buffer);
}


void printFast(int x, int y, String str, int16_t color, uint16_t bg){
 char c, i=0;
  while (1){
    c =  str[i++];
    if (!c) break;
    drawCharFast(x, y, c, color, bg);
    x += 6;
  }
}



uint16_t checkKey(){
  keyState = ~mcp.readGPIOAB() & 255;
  return (keyState);
}


uint32_t waitKeyUnpressed(){
  uint32_t timerStamp = millis();
  while (checkKey() && (millis()-timerStamp) < KEY_UNPRESSED_TIMEOUT) delay(1);
  return (millis() - timerStamp);
}


void redrawOnscreen(uint8_t slX, uint8_t slY){
  tft.fillRect(0, 128 - 24, 128, 24, TFT_BLACK);
  for (uint8_t i=0; i<20; i++) drawCharFast(i*6+4, 128-24-2, pgm_read_byte(&keybOnscr[0][i]), TFT_YELLOW, TFT_BLACK); 
  for (uint8_t i=0; i<20; i++) drawCharFast(i*6+4, 128-16-2, pgm_read_byte(&keybOnscr[1][i]), TFT_YELLOW, TFT_BLACK); 
  for (uint8_t i=0; i<20; i++) drawCharFast(i*6+4, 128-8-2, pgm_read_byte(&keybOnscr[2][i]), TFT_YELLOW, TFT_BLACK); 
  drawCharFast(slX*6+4, 128-24+slY*8-2, pgm_read_byte(&keybOnscr[slY][slX]), TFT_RED, TFT_BLACK); 
  drawCharFast(6*19+4, 128-24+2*8-2, 'E', TFT_WHITE, TFT_BLACK); 
  drawCharFast(6*18+4, 128-24+2*8-2, '<', TFT_WHITE, TFT_BLACK); 
}


void redrawSelected (uint8_t slX, uint8_t slY){
 static uint8_t prevX = 0, prevY = 0;
  drawCharFast(prevX*6+4, 128-24+prevY*8-2, pgm_read_byte(&keybOnscr[prevY][prevX]), TFT_YELLOW, TFT_BLACK); 
  drawCharFast(6*19+4, 128-24+2*8-2, 'E', TFT_WHITE, TFT_BLACK); 
  drawCharFast(6*18+4, 128-24+2*8-2, '<', TFT_WHITE, TFT_BLACK);
  drawCharFast(slX*6+4, 128-24+slY*8-2, pgm_read_byte(&keybOnscr[slY][slX]), TFT_RED, TFT_BLACK); 
  prevX = slX; 
  prevY = slY; 
}


void drawTyping(){
    tft.fillRect(1, 128-5*8, 126, 8, TFT_BLACK);  
    if(typing.length()<20) printFast(4, 128-5*8, typing+cursorType[cursorTypeFlag], TFT_WHITE, TFT_BLACK);
    else printFast(4, 128-5*8, "<"+typing.substring(typing.length()-18)+cursorType[cursorTypeFlag], TFT_WHITE, TFT_BLACK);
}


void drawBlinkingCursor(){
   if (millis() > cursorBlinkMillis+CURSOR_BLINKING_PERIOD){
      cursorBlinkMillis = millis();
      cursorTypeFlag = !cursorTypeFlag;
      drawTyping();
   }
}


void keybOnscreen(){
   if (checkKey()){
      lcdMaxBright();
      tone(SOUNDPIN, 100, 10);

      if(keyState&PAD_RGT){
         GSM.callAnswer();
         drawConsole("Answer",TFT_WHITE);
         drawConsole(GSM.getCommand(),TFT_MAGENTA);
         drawConsole(GSM.getAnswer(),TFT_YELLOW);
      }
      
      if(keyState&PAD_LFT){
         GSM.callHangoff();
         drawConsole("Hang off",TFT_WHITE);
         drawConsole(GSM.getCommand(),TFT_MAGENTA);
         drawConsole(GSM.getAnswer(),TFT_YELLOW);
      }
         
      if ((keyState&PAD_RIGHT) && selX < 20)  selX++; 
      if ((keyState&PAD_LEFT) && selX > -1)    selX--;
      if ((keyState&PAD_DOWN) && selY < 3)    selY++;
      if ((keyState&PAD_UP) && selY > -1)      selY--; 
      if ((keyState&PAD_LEFT) && selX == -1)   selX=19;
      if ((keyState&PAD_RIGHT) && selX == 20) selX=0;
      if ((keyState&PAD_UP) && selY == -1)     selY=2; 
      if ((keyState&PAD_DOWN) && selY == 3)   selY=0;
      
      if ((((keyState&PAD_ACT) && (selX == 19 && selY == 2)) || (keyState&PAD_ACT && keyState&PAD_ESC) || (keyState&PAD_RGT && keyState&PAD_LFT)) && typing.length()>0){//enter
        sendFlag = 1;
        } 
      else
        if (((keyState&PAD_ACT) && (selX == 18 && selY == 2) || (keyState&PAD_ESC)) && typing.length()>0){//back space
            typing.remove(typing.length()-1); 
        } 
        else
          if ((keyState&PAD_ACT) && (selX == 17 && selY == 1) && typing.length() < 19){//SPACE
            waitKeyUnpressed();
            typing += " "; 
          } 
          else
            if (keyState&PAD_ACT && typing.length() < MAX_TYPING_CHARS) {
              if (waitKeyUnpressed() > KEY_PRESSED_DELAY_TO_SEND)  sendFlag = 1;
              else typing += (char)pgm_read_byte(&keybOnscr[selY][selX]); 
            }
   redrawSelected (selX, selY);
   drawTyping();
   }
  drawBlinkingCursor();
}




void setup() {
  Serial.begin(9600);                           // Скорость обмена данными с компьютером
  
  WiFi.mode(WIFI_OFF);
  delay(100);

//LED init
  myled.begin();

//DAC init, LCD backlit off
  dac.begin(MCP4725address);
  delay(50);
  dac.setVoltage(0, false);
  delay(100);

//MCP23017 and buttons init, should preceed the TFT init
  mcp.begin(MCP23017address);
  delay(100);

  for (int i = 0; i < 8; ++i){
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }

//Sound init and test
  pinMode(SOUNDPIN, OUTPUT);
  tone(SOUNDPIN, 200, 100);
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);

//TFT init
  mcp.pinMode(CSTFTPIN, OUTPUT);
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(200);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

//VIBRO init
  mcp.pinMode(VIBROPIN, OUTPUT);
  mcp.digitalWrite(VIBROPIN, LOW);

//draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(38, 95);
  tft.print(F("GSM phone"));

//LCD backlit on
  for (uint8_t bcklt=0; bcklt<100; bcklt++){
    dac.setVoltage(bcklt*20, false);
    delay(10);}
  dac.setVoltage(4095, true);

//init ADC voltage meter
 pinMode(A0,INPUT); 

//clear screen and reset LCDfadeTimer
 tft.fillScreen(TFT_BLACK);
 lcdFadeBrightness = 4095;
 lcdFadeTimer = millis();
 delay(200);

 
//draw interface
 tft.fillRect(0, 128-24, 128, 24, TFT_BLACK);
 redrawOnscreen(selX, selY);
 drawTyping();
 tft.drawRect(0, 128-3*8-5, 128, 3*8+5, TFT_NAVY);
 tft.drawRect(0, 0, 128, MAX_CONSOLE_STRINGS*8+4, TFT_NAVY);
 tft.drawRect(0, 0, 128, 128, TFT_NAVY);


 //GSM init
  drawConsole(F("use SIM800 AT commnd"), TFT_BLUE);
  drawConsole(F("RGT-answer, LFT-hang"), TFT_BLUE);
  drawConsole(F("<telno> for call"), TFT_BLUE);
  drawConsole(F("<tel>,<text> for sms"), TFT_BLUE);
  drawConsole(F("long press A to entr"), TFT_BLUE);
  drawConsole(F(" "), TFT_YELLOW);
 drawConsole(F("init GSM..."), TFT_WHITE);
 myled.setRGB(20,20,0);
 if(GSM.init(9600)) drawConsole("OK", TFT_GREEN);
 else { 
   drawConsole(F("FAULT"), TFT_RED); 
   myled.setRGB(4,0,0); 
   while(1) delay(100);
 }
 if(!GSM.isSimInserted()) drawConsole(F("No SIM"), TFT_RED);
 else drawConsole(F("searching network..."), TFT_WHITE);
 GSM.setErrorReport(2);
 GSM.setURC(true);
 GSM.setSIManswerStyle(false);
 GSM.callSetCOLP(true);
 GSM.smsTextMode(true);
 GSM.setAnswer(true);
 GSM.setEcho(false);
 GSM.setAutoAnswer (false);
 GSM.setRingerVolume(100);
 GSM.setSpeakerVolume(100);
 GSM.setPhoneFunc(1);
 GSM.setTimeFromOperator(true);
 GSM.saveParam ();
 
 while (!GSM.isRegistered()) delay(50);
 myled.setRGB(0,20,0);
 tone(SOUNDPIN,400, 100);
 mcp.digitalWrite(VIBROPIN, HIGH);
 
 drawConsole(GSM.operatorName(), TFT_GREEN);
 drawConsole("RSSI:" + (String)GSM.signalQuality(), TFT_WHITE);
}


void lcdMaxBright(){
    lcdFadeTimer = millis();
    lcdFadeBrightness = 4095;
    dac.setVoltage(lcdFadeBrightness, false);
}


void loop(){
 static uint32_t availableDelay = 0;
 
  if (sendFlag) {
    lcdMaxBright();
    sendFlag = 0;
    tone(SOUNDPIN,200, 100);
    myled.setRGB(0,10,0);

    if (typing[0] == '+' && typing[1] >='0' && typing[1] <='9' && typing[2] >='0' && typing[2] <='9'){
      if (typing.indexOf(',') == -1) {
        drawConsole("CALLING...",TFT_WHITE);
        myled.setRGB(0,10,0);
        GSM.call((char*)typing.c_str());
        drawConsole(GSM.getCommand(),TFT_MAGENTA);
        typing = "";
      }
      else  {
        drawConsole("SMS sending...",TFT_WHITE);
        myled.setRGB(0,10,0);
        GSM.smsSend((char *)(typing.substring(0,typing.indexOf(','))).c_str(), (char *)(typing.substring(typing.indexOf(',')+1)).c_str());
        drawConsole(GSM.getCommand(),TFT_MAGENTA);
        delay(500);
        GSM.smsDeleteAll();
        typing = "";
      }
    }
    else {
      GSM.sendCommand(typing, true);
      drawConsole(GSM.getCommand(),TFT_MAGENTA);
      drawConsole(GSM.getAnswer(),TFT_YELLOW);
      if (GSM.getAnswer().indexOf("OK") !=-1){
        typing = "";
      }
    }
  }

  if (millis() > availableDelay+2000){
    availableDelay = millis();
    if (!lcdFadeBrightness && !myled.getRGB()) myled.setRGB(0,0,20);
  }

  if (GSM.available()) {
      lcdMaxBright();
      mcp.digitalWrite(VIBROPIN, HIGH);
      tone(SOUNDPIN,400, 100);
      myled.setRGB(0,10,0);
      String getGSManswer = GSM._read();
      
      drawConsole(getGSManswer,TFT_GREEN);
      if (getGSManswer.indexOf("+CLIP") != -1){ 
        drawConsole("CALL " + getGSManswer.substring(getGSManswer.indexOf("+CLIP")+8, getGSManswer.indexOf("\",")),TFT_WHITE);
      }
      else 
        if (getGSManswer.indexOf("RING") != -1) drawConsole("INCOMING CALL",TFT_WHITE);
      if (getGSManswer.indexOf("+CMTI") != -1){
        drawConsole(GSM.smsRead(1, 1),TFT_WHITE);
        GSM.smsDeleteAll();
      } 
  }


  if ((millis() > (lcdFadeTimer + TFT_FADEOUT_DELAY)) && (lcdFadeBrightness > 0)){
      lcdFadeBrightness -= 100;
      if (lcdFadeBrightness < 0) lcdFadeBrightness = 0;
      dac.setVoltage(lcdFadeBrightness, false);
  }
    
  keybOnscreen();
  delay(70);
  if (myled.getRGB()) myled.setRGB(0,0,0);
  if (mcp.digitalRead(VIBROPIN)) mcp.digitalWrite(VIBROPIN, LOW);
}
