/*
	ESPboyGSM Library
	
	This library written for ESPboy GSM phone shield ESPboy project by RomanS
    ESPboy.edu@gmail.com

	based on ideas of GSMSim Library by Erdem ARSLAN and SIM800 documentation.
	erdemsaid@gmail.com
*/


#include "Arduino.h"
#include "ESPboyGSMlib.h"
#include <SoftwareSerial.h>


ESPboyGSM::ESPboyGSM(uint8_t rx, uint8_t tx) : SoftwareSerial(rx, tx){
	RX_PIN = rx;
	TX_PIN = tx;
}



// Start GSM
bool ESPboyGSM::init(uint32_t baud) {
	_baud = baud;
	this->begin(_baud);
	this->setTimeout(1000);
	delay(500);
	_buffer.reserve(BUFFER_RESERVE_MEMORY);
	_command.reserve(COMMAND_RESERVE_MEMORY);
	_command = "AT";
	this->print(_command + "\r");
	_buffer = _readSerial();
    if( (_buffer.indexOf("OK") ) != -1)  return(true);
	else return(false);
}



bool ESPboyGSM::sendCommand(String comm, bool waitOK){
    _command = comm;
	this->print(_command + "\r");
	delay(500);
	if (waitOK == true){	
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1) return(true);
		else return(false);
	}
	else return(true);
}



//serial available
bool ESPboyGSM::_available(){
   return(this->available());
}


//serial read
String ESPboyGSM::_read(){
String readSerial;
   readSerial = this->readString();
   
   for (uint16_t i=0; i<readSerial.length(); i++){
     if ((readSerial[i] <  ' ') || (readSerial[i] > 'z'))
       readSerial[i] = ' ';
   }
   readSerial.replace("  ", " ");
   readSerial.replace("  ", " ");
   readSerial.trim();
   return readSerial;
}


String ESPboyGSM::getAnswer(){
  for (uint16_t i=0; i<_buffer.length(); i++){
    if ((_buffer[i] <  ' ') || (_buffer[i] > 'z'))
      _buffer[i] = ' ';
  }
 _buffer.replace("  ", " ");
 _buffer.replace("  ", " ");
 _buffer.trim();
 return _buffer;
}


String ESPboyGSM::getCommand(){
 return _command;
}


//SIM800 error answer: true=numerical, false=text
bool ESPboyGSM::setSIManswerStyle(bool type) {
        if (type == true){
            _command = "ATV0";
			this->print(_command + "\r");
		}
		else{
		    _command = "ATV1";
			this->print(_command + "\r");
		}
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1) return(true);
		else return(false);
}


//SIM800 URC mode true-on, false-off
bool ESPboyGSM::setURC(bool type) {
		if (type == true){
		    _command = "AT+CLIP=1";
			this->print(_command + "\r");
		}
		else{
		    _command = "AT+CLIP=0";
			this->print(_command + "\r");
		}
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1) return(true);
		else return(false);
}


//Error report level 0="ERROR", 1=error code, 2=error text description
bool ESPboyGSM::setErrorReport(uint8_t level) {
	if(level != 0 || level != 1 || level != 1) {
		return(false);
	}
	else {
	    _command = "AT+CMEE=" + String(level);
		this->print(_command + "\r");
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1)  {
			return(true);
		}
		else {
			return(false);
		}
	}
}


// ECHO true=on, false=off
bool ESPboyGSM::setEcho(bool type) {
    if (type == true){
        _command = "ATE1";
    	this->print(_command + "\r");
    }
    else{
        _command = "ATE0";
		this->print(_command + "\r");
	}
	_buffer = _readSerial();
	if ( (_buffer.indexOf("OK") )!=-1 ) {
   		return(true);
   	}
   	else {
   		return(false);
   	}
}


// ANSWER true=on, false=off
bool ESPboyGSM::setAnswer(bool type) {
    if (type == true){
        _command = "ATV1";
    	this->print(_command + "\r");
    }
    else{
        _command = "ATV0";
		this->print(_command + "\r");
	}
	_buffer = _readSerial();
	if ( (_buffer.indexOf("OK") )!=-1 ) {
   		return(true);
   	}
   	else {
   		return(false);
   	}
}


// set phone functionality 0=min, 1=full, 4-...
bool ESPboyGSM::setPhoneFunc(uint8_t level) {
	if(level != 0 || level != 1 || level != 4) {
		return(false);
	}
	else {
	    _command = "AT+CFUN=" + String(level);
		this->print(_command + "\r");
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1)  {
			return(true);
		}
		else {
			return(false);
		}
	}
}


//save param
bool ESPboyGSM::saveParam () {
    _command = "AT&W";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if( (_buffer.indexOf("OK") ) != -1)  {
		return(true);
	}
	else {
		return(false);
	}
}


//get ringer volume [0-100]
uint8_t ESPboyGSM::ringerVolume() {
    _command = "AT+CRSL?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(7, _buffer.indexOf("OK"));
	veri.trim();
	return veri.toInt();
}


//set ringer volume [0-100]
bool ESPboyGSM::setRingerVolume(uint8_t level) {
	if(level > 100) {
		level = 100;
	}
	_command = "AT+CRSL=" + (String)level;
	this->print(_command + "\r");
	_buffer = _readSerial();
	if(_buffer.indexOf("OK") != -1) {
		return(true);
	} else {
		return(false);
	}
}


//get speaker volume [0-100]
uint8_t ESPboyGSM::speakerVolume() {
    _command = "AT+CLVL?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(7, _buffer.indexOf("OK"));
	veri.trim();
	return veri.toInt();
}


//set speaker volume [0-100]
bool ESPboyGSM::setSpeakerVolume(uint8_t level) {
	if(level > 100) {
		level = 100;
	}
    _command = "AT+CLVL=" + (String)level;
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else {
		return(false);
	}
}


//auto answer 0=no, n=after "n" number of rings
bool ESPboyGSM::setAutoAnswer (uint8_t level) {
    _command = "ATS0=" + String(level);
	this->print(_command + "\r");
	_buffer = _readSerial();
	if( (_buffer.indexOf("OK") ) != -1)  {
		return(true);
	}
	else {
		return(false);
	}
}


// signal quality - 0-31 / 99=Unknown
uint8_t ESPboyGSM::signalQuality() {
    _command = "AT+CSQ";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if((_buffer.indexOf("+CSQ:")) != -1) {
		return _buffer.substring(_buffer.indexOf("+CSQ: ")+6, _buffer.indexOf(",")).toInt();
	} else {
		return 99;
	}
}


// is module connected to the operator?
bool ESPboyGSM::isRegistered() {
    _command = "AT+CREG?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if( (_buffer.indexOf("+CREG: 0,1")) != -1 || (_buffer.indexOf("+CREG: 0,5")) != -1 || (_buffer.indexOf("+CREG: 1,1")) != -1 || (_buffer.indexOf("+CREG: 1,5")) != -1) {
		return(true);
	} else {
		return(false);
	}
}


// is SIM inserted?
bool ESPboyGSM::isSimInserted() {
    _command = "AT+CSMINS?";
	this->print(_command + "\r");
	delay(500);
	_buffer = _readSerial();
	if(_buffer.indexOf(",") != -1) {
		String veri = _buffer.substring(_buffer.indexOf(",")+1, _buffer.indexOf(",")+2 );
		veri.trim();	
		if(veri == "1") {
			return(true);
		} else {
			return(false);
		}
	} else {
		return(false);
	}
}


// operator name
String ESPboyGSM::operatorName() {
    _command = "AT+COPS?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if(_buffer.indexOf(",") == -1) {
		return F("Not connected");
	}
	else {
		 return _buffer.substring(_buffer.indexOf(",\"")+2, _buffer.lastIndexOf("\""));
	}
}

// operator name from SIM
String ESPboyGSM::operatorNameFromSim() {
	this->flush();
	_command = "AT+CSPN?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	delay(200);
	_buffer = _readSerial();
	if(_buffer.indexOf("OK") != -1) {
		return _buffer.substring(_buffer.indexOf(" \"") + 2, _buffer.lastIndexOf("\""));
	}
	else {
		return F("Not connected");
	}

}


/*phone status
0 Ready (MT allows commands from TA/TE)
2 Unknown (MT is not guaranteed to respond to instructions)
3 Ringing (MT is ready for commands from TA/TE, but the ringer is active)
4 Call in progress (MT is ready for commands from TA/TE, a call is in progress)
*/
uint8_t ESPboyGSM::phoneStatus() {
    _command = "AT+CPAS";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if((_buffer.indexOf("+CPAS: ")) != -1)
	{
		return _buffer.substring(_buffer.indexOf("+CPAS: ")+7,_buffer.indexOf("+CPAS: ")+9).toInt();
	}
	else {
		return 99; // not read from module
	}
}


//manufacturer identification
String ESPboyGSM::moduleManufacturer() {
    _command = "AT+CGMI";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	veri.trim();
	veri.replace("_", " ");
	return veri;
}


//module identification
String ESPboyGSM::moduleModel() {
    _command = "AT+CGMM";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	veri.trim();
	veri.replace("_", " ");
	return veri;
}


//revision identification of software release
String ESPboyGSM::moduleRevision() {
    _command = "AT+CGMR";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(_buffer.indexOf(":")+1 , _buffer.indexOf("OK"));
	veri.trim();
	return veri;
}


//module IMEI
String ESPboyGSM::moduleIMEI() {
    _command = "AT+CGSN";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	veri.trim();
	return veri;
}



//international mobile subscriber identity
String ESPboyGSM::moduleIMSI() {
    _command = "AT+CIMI";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	veri.trim();
	return veri;
}


//ICCID
String ESPboyGSM::moduleICCID() {
    _command = "AT+CCID";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String veri = _buffer.substring(8, _buffer.indexOf("OK"));
	veri.trim();
	return veri;
}



String ESPboyGSM::moduleDebug() {
    _command = "AT&V";
	this->print(_command + "\r");
	return _readSerial();
}


//////////
// CALL //
//////////

// call
// 0 - failed / 1 - ATD OK no COLP / 2 - BUSY / 3 - NO DIAL TONE / 4 - NO CARRIER / 5 - OK answered
uint8_t ESPboyGSM::call(char* phone_number) {
	bool colp = callIsCOLPActive();
	_buffer = _readSerial();
	delay(300);
	_command = "ATD" + (String)(phone_number)+";";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (colp) {
		if (_buffer.indexOf("BUSY") != -1){
			return 2;
		}
		if (_buffer.indexOf("NO DIAL TONE") != -1){
			return 3;
		}
		if (_buffer.indexOf("NO CARRIER") != -1){
			return 4;
		}
		if (_buffer.indexOf("OK") != -1){
			return 5;
		}	
	}
	else {
		if (_buffer.indexOf("OK") != -1){
			return 1;
		}
		else {
			return 0;
		}
	}
}


// answer
bool ESPboyGSM::callAnswer() {
    _command = "ATA";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1){
		return(true);
	}
	else {
		false;
	}
}


// hang off
bool ESPboyGSM::callHangoff() {
	_command = "ATH";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if(_buffer.indexOf("OK") != -1){
		return(true);
	} 
	else {
		false;
	}
}


/* call status
	0 - Ready (MT allows commands from TA/TE)
	2 - Unknown (MT is not guaranteed to respond to tructions)
	3 - Ringing (MT is ready for commands from TA/TE, but the ringer is active)
	4 - Call in progress */
uint8_t ESPboyGSM::callStatus() {
    _command = "AT+CPAS";
	this->print(_command + "\r");
	_buffer = _readSerial();
	return _buffer.substring(_buffer.indexOf("+CPAS: ") + 7, _buffer.indexOf("+CPAS: ") + 9).toInt();
}


// COLP=false, module returns OK just after ATD command
// COLP=true, return BUSY / NO DIAL TONE / NO CARRIER / OK - after taking call other side
bool ESPboyGSM::callSetCOLP(bool active) {
    _command = "AT+COLP=" + (String) active;
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1){
		return(true);
	}
	else {
		false;
	}
}


// is COLP active?
bool ESPboyGSM::callIsCOLPActive() {
    _command = "AT+COLP=?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf(",1") != -1) {
		return(true);
	}
	else {
		false;
	}
}



/////////
// SMS //
/////////

// SMS text mode on/off
bool ESPboyGSM::smsTextMode(bool textModeON) {
	if (textModeON == true) {
	    _command = "AT+CMGF=1";
		this->print(_command + "\r");
	}
	else {
	    _command = "AT+CMGF=0";
		this->print(_command + "\r");
	}
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else{
		return(false);
	}
}



// send SMS
bool ESPboyGSM::smsSend(char* number, char* message) {
    _command = "AT+CMGS=\"" + (String)number + "\"";
	this->print( _command + F("\r"));
	delay(300);
	_buffer = _readSerial();
	this->print(message);
	this->print(F("\r"));
	_buffer += _readSerial();
	this->print((char)26);
	_buffer += _readSerial();
	if (((_buffer.indexOf("+CMGS:")) != -1)) {
		if (((_buffer.indexOf("OK")) != -1)){
			return(true);
		}
		else {
			return(false);
		}
	}
	else {
		return(false);
	}
}


// return unread SMS
String ESPboyGSM::smsListUnread() {
    _command = "AT+CMGL=\"REC UNREAD\",1";
    this->print(_command + "\r");
	_buffer = _readSerial();
	String donus = "";
	if (_buffer.indexOf("ERROR") != -1) {
		donus = "ERROR";
	}
	if (_buffer.indexOf("+CMGL:") != -1) {
		String veri = _buffer;
		bool islem = false;
		donus = "";
		while (!islem) {
			if (veri.indexOf("+CMGL:") == -1) {
				islem = true;
				continue;
			}
			veri = veri.substring(veri.indexOf("+CMGL: ") + 7);
			String metin = veri.substring(0, veri.indexOf(","));
			metin.trim();
			if (donus == "") {
				donus += "SMSIndexNo:";
				donus += metin;
			}
			else {
				donus += ",";
				donus += metin;
			}
		}
	}
	else {
		if (donus != "ERROR") {
			donus = "NO_SMS";
		}
	}
	return donus;
}


// return SMS no = index
String ESPboyGSM::smsRead(uint8_t index, bool markRead) {
 String klasor, okundumu, telno, zaman, mesaj;
    _command = "AT+CMGR=" + (String)index + ",";
	if (markRead == true) _command += "0";
	else _command += "1";
	this->print(_command + "\r");
	_buffer = _readSerial();
	String durum = "INDEX_NO_ERROR";
	if (_buffer.indexOf("+CMGR:") != -1) {
		klasor = "UNKNOWN";
		okundumu = "UNKNOWN";
		if (_buffer.indexOf("REC UNREAD") != -1) {
			klasor = "INCOMING";
			okundumu = "UNREAD";
		}
		if (_buffer.indexOf("REC READ") != -1) {
			klasor = "INCOMING";
			okundumu = "READ";
		}
		if (_buffer.indexOf("STO UNSENT") != -1) {
			klasor = "OUTGOING";
			okundumu = "UNSENT";
		}
		if (_buffer.indexOf("STO SENT") != -1) {
			klasor = "OUTGOING";
			okundumu = "SENT";
		}
		String telno_bol1 = _buffer.substring(_buffer.indexOf("\",\"") + 3);
		telno = telno_bol1.substring(0, telno_bol1.indexOf("\",\""));
		String tarih_bol = telno_bol1.substring(telno_bol1.lastIndexOf("\",\"") + 3);
		zaman = tarih_bol.substring(0, tarih_bol.indexOf("\"")); 
		mesaj = tarih_bol.substring(tarih_bol.indexOf("\"")+1, tarih_bol.lastIndexOf("OK"));
		mesaj.trim();
		durum = "FOLDER:";
		durum += klasor;
		durum += "|STATUS:";
		durum += okundumu;
		durum += "|PHONENO:";
		durum += telno;
		durum += "|DATETIME:";
		durum += zaman;
		durum += "|MESSAGE:";
		durum += mesaj;
	}
	return durum;
}



// delete SMS no = index
bool ESPboyGSM::smsDeleteOne(uint8_t index) {
    _command = "AT+CMGD=" + (String)index + ",0";
    this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else {
		return(false);
	}
}


// delete all SMS been read
bool ESPboyGSM::smsDeleteAllRead() {
    _command = "AT+CMGD=1,1";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else {
		return(false);
	}
}

 

// delete all SMS
bool ESPboyGSM::smsDeleteAll() {
    _command = "AT+CMGDA=\"DEL ALL\"";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else {
		return(false);
	}
}



///////////////////
//	USSD SECTION //
///////////////////

bool ESPboyGSM::ussdSend(char* code) {
    _command = "AT+CUSD=1,\"" + (String)code + "\"";
    this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		return(true);
	}
	else {
		return(false);
	}
}


String ESPboyGSM::ussdRead(String serialRaw) {
	if (serialRaw.indexOf("+CUSD:") != -1) {
		String metin = serialRaw.substring(serialRaw.indexOf(",\"") + 2, serialRaw.indexOf("\","));
		return metin;
	}
	else {
		return "NOT_USSD_RAW";
	}
}



////////////////////
//	TIME SECTION  //
////////////////////

bool ESPboyGSM::setTimeFromOperator (bool optime){
		if (optime == true){
			_command = "AT+CLTS=1";
		}
		else{
			_command = "AT+CLTS=0";
		}
		this->print(_command + "\r");
		_buffer = _readSerial();
		if( (_buffer.indexOf("OK") ) != -1)  {
			return(true);
		}
		else {
			return(false);
		}
}



bool ESPboyGSM::timeGet(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second) {
	_command = "AT+CCLK?";
	this->print(_command + "\r");
	_buffer = _readSerial();
	if (_buffer.indexOf("OK") != -1) {
		_buffer = _buffer.substring(_buffer.indexOf("\"") + 1, _buffer.lastIndexOf("\"") - 1);
		*year = (_buffer.substring(0, 2).toInt()) + 2000;
		*month = _buffer.substring(3, 5).toInt();
		*day = _buffer.substring(6, 8).toInt();
		*hour = _buffer.substring(9, 11).toInt();
		*minute = _buffer.substring(12, 14).toInt();
		*second = _buffer.substring(15, 17).toInt();
		return(true);
	}
	else {
		return(false);}
}




String ESPboyGSM::_readSerial() {
	uint64_t timeOld = millis();
	while (!this->available() && !(millis() > timeOld + TIMEOUT_READ_SERIAL)){
		delay(10);
	}
	String str = "";
	if (this->available()){
		str = this->readString();
	}
	else str = "time out";
	str.trim();
	return str;
}


