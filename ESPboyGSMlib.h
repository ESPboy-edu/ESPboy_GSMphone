/*
	ESPboyGSM Library
	
	This library written for ESPboy GSM phone shield ESPboy project by RomanS
    ESPboy.edu@gmail.com

	based on ideas of GSMSim Library by Erdem ARSLAN and SIM800 documentation.
	erdemsaid@gmail.com
*/

#ifndef ESPboyGSM_h
#define ESPboyGSM_h

#include <SoftwareSerial.h>

#define BUFFER_RESERVE_MEMORY	255
#define COMMAND_RESERVE_MEMORY	255
#define TIMEOUT_READ_SERIAL		2000


//call status
#define CALL_STATUS_READY 		0
#define CALL_STATUS_UNKNOWN 	2
#define CALL_STATUS_RINGING 	3
#define CALL_STATUS_IN_CALL 	4
#define CALL_STATUS_NOT_READ 	99

//incoming events
#define EVENT_UNKNOWN				0
#define EVENT_INCOMING_RING 		1//"RING"
#define EVENT_INCOMING_NEW_SMS		2//"+CMTI"
#define EVENT_INCOMING_URC			3//"+CLIP"
#define EVENT_INCOMING_USSD 		4//"+CUSD"
#define EVENT_VOLTAGE_LOW_OFF		5//"UNDER-VOLTAGE POWER DOWN"
#define EVENT_VOLTAGE_LOW			6//"UNDER-VOLTAGE WARNNING"
#define EVENT_VOLTAGE_OVER_OFF		7//"OVER-VOLTAGE POWER DOWN"
#define EVENT_VOLTAGE_HIGH			8//"OVER-VOLTAGE WARNNING"
#define EVENT_TEMPERATURE_WARNING	9//"+CMTE"

#define ST_EVENT_UNKNOWN				0
#define ST_EVENT_INCOMING_RING 			F("RING");
#define ST_EVENT_INCOMING_NEW_SMS		F("+CMTI");
#define ST_EVENT_INCOMING_URC			F("+CLIP");
#define ST_EVENT_INCOMING_USSD 			F("+CUSD");
#define ST_EVENT_VOLTAGE_LOW_OFF		F("UNDER-VOLTAGE POWER DOWN");
#define ST_EVENT_VOLTAGE_LOW			F("UNDER-VOLTAGE WARNNING");
#define ST_EVENT_VOLTAGE_OVER_OFF		F("OVER-VOLTAGE POWER DOWN");
#define ST_EVENT_VOLTAGE_HIGH			F("OVER-VOLTAGE WARNNING");
#define ST_EVENT_TEMPERATURE_WARNING	F("+CMTE");


//class begins here
class ESPboyGSM : public SoftwareSerial
{
	private:
		uint32_t _baud;
		uint8_t _timeout;
		String _buffer;
		String _command;
		

	public:
		uint8_t	RX_PIN;
		uint8_t TX_PIN;
		uint32_t BAUDRATE;

//serial
		bool _available();
		String _read();
		String _readSerial();
		String getAnswer();
		String getCommand();
		
//base
		ESPboyGSM(uint8_t rx, uint8_t tx);
		bool init(uint32_t baud);
		bool sendCommand(String comm, bool waitOK);
		bool setSIManswerStyle(bool type);;
		bool setURC(bool type);
		bool setEcho(bool type);
		bool setAnswer(bool type);
		bool setErrorReport(uint8_t level);
		bool setPhoneFunc(uint8_t level);
		bool saveParam();
		uint8_t ringerVolume();
		bool setRingerVolume(uint8_t level);
		uint8_t speakerVolume();
		bool setSpeakerVolume(uint8_t level);
		bool setAutoAnswer (uint8_t level);
		
		uint8_t signalQuality();
		bool isRegistered();
		bool isSimInserted();
		
		uint8_t phoneStatus();
		
		String operatorName();
		String operatorNameFromSim();
		String moduleManufacturer();
		String moduleModel();
		String moduleRevision();
		String moduleIMEI();
		String moduleIMSI();
		String moduleICCID();
		String moduleDebug();

//call
  	    uint8_t call(char* phone_number);
		bool callAnswer();
		bool callHangoff();
		uint8_t callStatus();
		bool callSetCOLP(bool active);
		bool callIsCOLPActive();

//SMS
		bool smsTextMode(bool textModeON);
		bool smsSend(char* number, char* message);
		String smsListUnread();
		String smsRead(uint8_t index, bool markRead);
		bool smsDeleteOne(uint8_t index);
		bool smsDeleteAllRead();
		bool smsDeleteAll();

//ussd
		String ussdRead(String serialRaw);
		bool ussdSend(char* code);

//time
		bool setTimeFromOperator (bool optime);
        bool timeGet(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second);
};

#endif
