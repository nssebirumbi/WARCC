#include "Particle.h"
//#include "sdfat/SdFat.h" /*the sdfat folder is in the base folder with the .ino file*/
//#include <ctype.h>
//#include "application.h"

SYSTEM_MODE(MANUAL); /*do not autoconnect to particle cloud*/
SYSTEM_THREAD(ENABLED);
STARTUP(cellular_credentials_set("internet", "", "", NULL));

String data="", errormsg="", systime="";
int rep_count=0,delcount=0, ind=0,filesize=0;
double SOC=100.0;
bool newdata=false, client_connected=false, cellular_is_on=false;

/*TCP configuration*/
TCPClient client;
String server = "wimea.mak.ug"; //default server
int port=10024; // default port
//char buff [1024];  /*see transmission metrics regarding choice of 1024*/

//==============================================================================================
void setup() {
	//some record to be sent to the server in the exact format as the other stations send
	byte buff=(byte)"RTC_T=2020-03-09,12:22:00 &: NAME=mkn-10m E64=fcc64400 FCC=40.7 V_IN=6.66 V_A1=2.21 [ADDR=252.4 SEQ=90.02 TTL=320.0 RSSI=12.112 LQI=4.098]";
	Serial1.begin(38400);
	Cellular.on();
	while(!Cellular.ready()){
		Serial.println("Connecting to cellular ...");
	} //wait until condition is true
	Serial.println(" === CONNECTED === .");
}

void loop() {
	while(Cellular.ready()){
		//************************************************************
		client.write(buff, sizeof(buf));//write string to server
		Serial.println(" === just sent data:  === .");
		Serial.println(buff);
		//************************************************************
	}
	Serial.println(" === !! Not Connected to cellular !! === .");
	setup();
	//Cellular.off();
}
