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

#define MAX_CONTACTS_STORE 100
#define MAX_SMS_STORE 200
#define MAX_CONSOLE_STRINGS 10
#define CURSOR_BLINKING_PERIOD 500
#define TFT_FADEOUT_DELAY 50000

Adafruit_MCP23017 mcp;
Adafruit_MCP4725 dac;
ESPboyLED myled;
TFT_eSPI tft = TFT_eSPI();
ESPboyGSM GSM(GSM_RX, GSM_TX);       // RX, TX


struct contact{
  char nme[24];
  char no[24];
};

struct sms{
  char text[140];
  uint32_t timestmp;
  uint8_t type;
};

static contact GSMcont[MAX_CONTACTS_STORE];
static sms GSMsms[MAX_SMS_STORE];
static uint16_t GSMsmsCount = 0;

constexpr uint8_t keybOnscr[3][21] PROGMEM = {
"1234567890ABCDEFGHIJ",
"KLMNOPQRSTUVWXYZ_+-=",
"?!@#$%&*()_[]\":;.,<E",
};

static String consolestrings[MAX_CONSOLE_STRINGS+1];
static uint16_t consolestringscolor[MAX_CONSOLE_STRINGS+1];
uint16_t line_buffer[46];
uint8_t keyState = 0, selX = 0, selY = 0;

String typing = "";
uint32_t cursorBlinkMillis = 0;
uint8_t cursorTypeFlag = 0;
char cursorType[2]={220,'_'};
uint8_t sendFlag = 0;

uint8_t lcdMaxBrightFlag;
int16_t lcdFadeBrightness;
uint32_t lcdFadeTimer;

int16_t GSMringFlag;
uint32_t GSMringTimer;
int16_t GSMsmsFlag;
uint32_t GSMsmsTimer;


String gsmResponse = ""; 


void drawConsole(String bfrstr, uint16_t color){
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



int checkKey(){
  keyState = ~mcp.readGPIOAB() & 255;
  return (keyState);
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
    printFast(4, 128-5*8, typing+cursorType[cursorTypeFlag], TFT_WHITE, TFT_BLACK);
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
      lcdMaxBrightFlag++;
      if ((keyState&PAD_RIGHT) && selX < 19) { selX++; redrawSelected (selX, selY); }
      if ((keyState&PAD_LEFT) && selX > 0) { selX--; redrawSelected (selX, selY); }
      if ((keyState&PAD_DOWN) && selY < 2) { selY++; redrawSelected (selX, selY); }
      if ((keyState&PAD_UP) && selY > 0) { selY--; redrawSelected (selX, selY); }
      
      if ((((keyState&PAD_ACT) && (selX == 19 && selY == 2)) || (keyState&PAD_ACT && keyState&PAD_ESC) || (keyState&PAD_RGT)) && typing.length()>0){//enter
        sendFlag = 1;
        } 
      else
        if (((keyState&PAD_ACT) && (selX == 18 && selY == 2) || (keyState&PAD_ESC)) && typing.length()>0){//back space
            typing.remove(typing.length()-1); 
            drawTyping();
        } 
        else
          if ((keyState&PAD_ACT) && (selX == 16 && selY == 1) && typing.length() < 19){//SPACE
            typing += " "; 
            drawTyping();
          } 
          else
            if (keyState&PAD_ACT && typing.length() < 19) {
            typing += (char)pgm_read_byte(&keybOnscr[selY][selX]); 
            drawTyping();
            while (checkKey()) delay(2); 
            }
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
 drawConsole(F("init GSM..."), TFT_WHITE);
 if(GSM.init(9600)) drawConsole(F("OK"), TFT_GREEN);
 else { drawConsole(F("FAULT"), TFT_RED); while(1) delay(100);}
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
 
 while (!GSM.isRegistered()) delay(100);
 drawConsole(GSM.operatorName(), TFT_GREEN);
}



void loop(){
 static uint32_t availableDelay = 0;
  if (sendFlag) {
    lcdMaxBrightFlag++;
    sendFlag = 0;
    tone(SOUNDPIN,200, 100);
    GSM.sendCommand(typing, true);
    drawConsole(GSM.getCommand(), TFT_MAGENTA);
//Serial.println(GSM.getAnswer());
    for (uint8_t i=0; i<=(GSM.getAnswer().length()/20); i++)
      drawConsole(GSM.getAnswer().substring(i*20),TFT_YELLOW);
//    typing = "";
    tft.fillRect(1, 128-5*8, 126, 8, TFT_BLACK); 
  }

  if (millis() > availableDelay+3000){
    availableDelay = millis();
    if (!lcdFadeBrightness && !myled.getRGB()) myled.setRGB(0,0,2);
  }


 if (GSM.available()) {
      lcdMaxBrightFlag++;
      String getGSManswer = GSM._read();
//Serial.println(getGSManswer);
      for (uint8_t i=0; i<=(getGSManswer.length()/20); i++)
        drawConsole(getGSManswer.substring(i*20),TFT_GREEN);
  }

  if (lcdMaxBrightFlag){
    lcdFadeTimer = millis();
    lcdMaxBrightFlag = 0;
    lcdFadeBrightness = 4095;
    dac.setVoltage(lcdFadeBrightness, false);
  }

  if ((millis() > (lcdFadeTimer + TFT_FADEOUT_DELAY)) && (lcdFadeBrightness > 0)){
      lcdFadeBrightness -= 100;
      if (lcdFadeBrightness < 0) lcdFadeBrightness = 0;
      dac.setVoltage(lcdFadeBrightness, false);
    }
    
  keybOnscreen();
  delay(75);
  if (myled.getRGB()) myled.setRGB(0,0,0);
}
