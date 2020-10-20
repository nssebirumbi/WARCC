/*
 * Project Test
 * Description:
 * Author:
 * Date:
 */
#include "Particle.h"
#include "SerialBufferRK.h"
#include <string.h>

SYSTEM_MODE(MANUAL); /*do not autoconnect to particle cloud*/
SYSTEM_THREAD(ENABLED);
STARTUP(cellular_credentials_set("internet", "", "", NULL));

SerialBuffer<256> serBuf(Serial1);

/*TCP configuration*/
TCPClient client, clientx;
String server = "wimea.mak.ac.ug"; //default server
int port = 10028; // default port
byte buff [1024];

CellularSignal sig;
int charsRead = 0, p = 0, buffer_printed = 0;
int read_check_printed = 0, newdata = 0;

bool cellular_is_on = false, client_connected = false;

#define BUFFER_SIZE 64

size_t buf_offset = 0;
char buffer[BUFFER_SIZE];
char buf[BUFFER_SIZE];
char identity[] = {'i', 'd', 'e', 'n', 't','i','t','y',' ','E','n','t','e','b','b','e'};

Thread* superVthread;
unsigned long lastTime = 0, now = 0;

os_thread_return_t sysreset(void* param){
	for(;;){
		if(cellular_is_on){
			now = millis();
			if((now-lastTime > 100000) && client_connected == false)
			{
				System.reset();
			}
		}
		delay(1000);
	}
}

// setup() runs once, when the device is first turned on.
void setup() {
	Serial1.begin(38400);
	Cellular.on();
	Cellular.connect();
	while(!Cellular.ready());
	if(client.connect(server, port)) {
		size_t result = client.write((const uint8_t*)identity, sizeof(identity));
	}
	
	superVthread = new Thread("superVthread", sysreset, NULL);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
	
	p = 0;
	// Empty the buffer
	for(int i = 0; i < sizeof(buffer); i++) {
		buffer[i] = '\0';
	}
	
	// while(1) {
		// Read data from the TCP client if its available
		while(client.available() && client.connected()) {
			char c = client.read();
			if(p < BUFFER_SIZE) {
				buffer[p++] = c;
			}
			else {
				buffer[p++] = '\n';
				buffer[p++] = '\0';
			}
			newdata = 1;
		}

		// Transmit the data to the Sink Node
		if(newdata > 0) {
			Serial.printlnf("Received: %s", buffer);
			Serial1.write(buffer);
			Serial1.write("\n");
			newdata = 0;
			memset(buffer, 0, sizeof(buffer));
		}

		// Read Serial Data from Sink Node if its available
		if(Serial1.available() >= 0) {
			while(Serial1.available()){
				if(buf_offset < BUFFER_SIZE) {
					char c = Serial1.read();
					if(c != '\n') {
						// Add character to buffer
						buf[buf_offset++] = c;
					}
					else {
						// End of line character found, Process the line
						buf[buf_offset] = 0;
						Serial.printlnf("Received From Sink: %s\n", buf);
						size_t res = client.write((const uint8_t*)buf, sizeof(buf)); // Send to TCP client
						buf_offset = 0;
					}
				}
				else {
					Serial.println("Buffer Overflow");
					buf_offset = 0;
				}
			}
		}
	// }
}