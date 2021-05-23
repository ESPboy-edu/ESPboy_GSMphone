//ESPboy GSM phone module. Using SIM800 chipset. 
//For www.ESPboy.com project. By RomanS 16.05.2021


#include "lib/User_Setup.h"
#define USER_SETUP_LOADED
#define RING_TIMER_RESET 5000

#include "lib/ESPboyGSMlib.h"
#include "lib/ESPboyGSMlib.cpp"
#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include "lib/ESPboyTerminalGUImod.h"
#include "lib/ESPboyTerminalGUImod.cpp"

#define LED_PIN  D4

ESPboyInit myESPboy;
ESPboyTerminalGUI terminalGUIobj(&myESPboy.tft, &myESPboy.mcp);
ESPboyGSM GSM(D4, D6);       // RX, TX
//ADC_MODE(ADC_VCC);


void setRGB(uint8_t rLED, uint8_t gLED, uint8_t bLED){
  pinMode(LED_PIN, OUTPUT);
  myESPboy.myLED.setRGB(rLED,gLED,bLED); 
  pinMode(LED_PIN, INPUT);
}


void setup() {
  Serial.begin(115200);    
  myESPboy.begin("GSM phone");
  terminalGUIobj.toggleDisplayMode(1);
  
//GSM init
 terminalGUIobj.printConsole(F("Use SIM800 AT commnd"), TFT_BLUE, 1, 0);
 terminalGUIobj.printConsole(F("RGT-answer, LFT-hang"), TFT_BLUE, 1, 0);
 terminalGUIobj.printConsole(F("<telno> for call"), TFT_BLUE, 1, 0);
 terminalGUIobj.printConsole(F("<tel>,<text> for sms"), TFT_BLUE, 1, 0);
 terminalGUIobj.printConsole(F("long press A to entr"), TFT_BLUE, 1, 0);
 terminalGUIobj.printConsole(F(""), TFT_YELLOW,1,0);
  
 terminalGUIobj.printConsole(F("Init GSM module..."), TFT_WHITE, 1, 0);
 setRGB(5,5,0);
 if(GSM.init(9600)) 
   terminalGUIobj.printConsole("OK", TFT_GREEN,1,0);
 else { 
   terminalGUIobj.printConsole(F("FAULT"), TFT_RED,1,0); 
   setRGB(4,0,0); 
   while(1) delay(500);
 }
 if(!GSM.isSimInserted()) {
  terminalGUIobj.printConsole(F("No SIM"), TFT_RED, 1, 0);
  setRGB(5,0,0);
  while(1) delay(100);
 }
 else terminalGUIobj.printConsole(F("Searching network..."), TFT_WHITE, 1, 0);
 
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
 setRGB(0,5,0);
 myESPboy.playTone(400, 100);
 
 terminalGUIobj.printConsole(GSM.operatorName(), TFT_GREEN, 1, 0);
 terminalGUIobj.printConsole("RSSI:" + (String)GSM.signalQuality(), TFT_WHITE, 1, 0);
}



void loop(){
  static uint32_t keysState;
  static uint32_t ringTimer;
  static String getGSManswer;
  static String typing;

  keysState = myESPboy.getKeys();
  
  if (keysState && !(keysState&PAD_LFT) && !(keysState&PAD_RGT)) {
    myESPboy.playTone(200, 100);
    setRGB(0,0,5);
    typing = terminalGUIobj.getUserInput();
    while(myESPboy.getKeys())delay(100);
    terminalGUIobj.toggleDisplayMode(1);

    if (typing[0] == '+' && typing[1] >='0' && typing[1] <='9' && typing[2] >='0' && typing[2] <='9'){
      if (typing.indexOf(',') == -1) {
        terminalGUIobj.printConsole("CALLING...",TFT_WHITE,1,0);
        setRGB(0,5,0);
        GSM.call((char*)typing.c_str());
        terminalGUIobj.printConsole(GSM.getCommand(),TFT_MAGENTA,1,0);
        typing = "";
      }
      else  {
        terminalGUIobj.printConsole("SMS sending...",TFT_WHITE,1,0);
        setRGB(0,5,0);
        GSM.smsSend((char *)(typing.substring(0,typing.indexOf(','))).c_str(), (char *)(typing.substring(typing.indexOf(',')+1)).c_str());
        terminalGUIobj.printConsole(GSM.getCommand(),TFT_MAGENTA,1,0);
        delay(500);
        GSM.smsDeleteAll();
        typing = "";
      }
    }
    else
      if(typing.length()){
        GSM.sendCommand(typing, true);
        terminalGUIobj.printConsole(GSM.getCommand(),TFT_MAGENTA,1,0);
        terminalGUIobj.printConsole(GSM.getAnswer(),TFT_YELLOW,1,0);
        if (GSM.getAnswer().indexOf("OK") !=-1)
          typing = "";
      }
  }

  if (GSM.available()) {
      myESPboy.playTone(400, 100);
      setRGB(0,5,0);
      getGSManswer = GSM._read();
      
      terminalGUIobj.printConsole(getGSManswer,TFT_GREEN,1,0);
      if (getGSManswer.indexOf("+CLIP") != -1){
        terminalGUIobj.printConsole("CALL " + getGSManswer.substring(getGSManswer.indexOf("+CLIP")+8, getGSManswer.indexOf("\",")),TFT_WHITE,1,0);
        terminalGUIobj.printConsole("RGT-answer LFT-hang",TFT_BLUE,1,0);
        ringTimer=millis();
      }
      if (getGSManswer.indexOf("+CMTI") != -1){
        terminalGUIobj.printConsole(GSM.smsRead(1, 1),TFT_WHITE, 1, 0);
        GSM.smsDeleteAll();
      } 
  }


   if(keysState&PAD_RGT && millis()-ringTimer<RING_TIMER_RESET){
         GSM.callAnswer();
         terminalGUIobj.printConsole(F("Answer"),TFT_WHITE,1,0);
         terminalGUIobj.printConsole(GSM.getCommand(),TFT_MAGENTA,1,0);
         terminalGUIobj.printConsole(GSM.getAnswer(),TFT_YELLOW,1,0);
   }

      
   if(keysState&PAD_LFT && millis()-ringTimer<RING_TIMER_RESET){
         GSM.callHangoff();
         terminalGUIobj.printConsole(F("Hang off"),TFT_WHITE,1,0);
         terminalGUIobj.printConsole(GSM.getCommand(),TFT_MAGENTA,1,0);
         terminalGUIobj.printConsole(GSM.getAnswer(),TFT_YELLOW,1,0);
   }
         
   if (millis()-ringTimer>RING_TIMER_RESET && (keysState&PAD_LFT || keysState&PAD_RGT)) 
     terminalGUIobj.doScroll();   
   else delay(100);

   if (myESPboy.myLED.getRGB()){
     setRGB(0,0,0);
   }
}
