/**
* \file
*         WIMEA-ICT Gen3 AWS Gateway
* \details
*   ATMEGA256RFR2 RSS2 MOTE with RTC, SD card and Electron 3G uplink + Asynchronous scheduling
* \author
*         Maximus Byamukama <maximus.byamukama@cedat.mak.ac.ug>
*/
#include <stdio.h>
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "contiki.h"
#include "rss2.h"
#include "dev/leds.h"

#define FOSC 16000000// Clock Speed
#define BAUD 38400
#define MYUBRR FOSC/16/BAUD-1
#define BUFFER_SIZE 1024

#define D_UDR0   UDR0
#define D_UDRE0M (1 << UDRE0)
#define D_UBRR0H UBRR0H
#define D_UBRR0L UBRR0L
#define D_UCSR0A UCSR0A
#define D_UCSR0B UCSR0B
#define D_UCSR0C UCSR0C
#define D_USART0_RX_vect USART0_RX_vect
#define D_USART0_TX_vect USART0_TX_vect

#define USART_MODE_ASYNC 0x00
#define USART_STOP_BITS_1 0x00
#define USART_PARITY_NONE 0x00
#define USART_BAUD_38400 25

#define USART_RECEIVER_ENABLE _BV (RXEN0)
#define USART_TRANSMITTER_ENABLE _BV (TXEN0)

#define USART_INTERRUPT_RX_COMPLETE _BV (RXCIE0)
#define USART_INTERRUPT_TX_COMPLETE _BV (TXCIE0)
#define USART_INTERRUPT_DATA_REG_EMPTY _BV (UDRIE0)

char received_data[BUFFER_SIZE];
int buffer_offset = 0;

PROCESS(init_process, "Initialization Process");
PROCESS(uart_process, "UART Process");
AUTOSTART_PROCESSES(&init_process, &uart_process);

PROCESS_THREAD(init_process, ev, data)
{
	PROCESS_BEGIN();
	
	D_UBRR0H = (uint8_t)(USART_BAUD_38400 >> 8);
    D_UBRR0L = (uint8_t)USART_BAUD_38400;

	D_UCSR0B = USART_RECEIVER_ENABLE | USART_TRANSMITTER_ENABLE | USART_INTERRUPT_RX_COMPLETE;

	D_UCSR0C = (1<<UCSZ00) | (1<<UCSZ01) | USART_MODE_ASYNC | USART_PARITY_NONE | USART_STOP_BITS_1;

	PROCESS_END();
}

PROCESS_THREAD(uart_process, ev, data)
{
	PROCESS_BEGIN();

	char send_data[] = {'H','e','l','l','o',' ','U','A','R','T'};

	for(;;) {
		while((UCSR0A & (1<<UDRE0)) == 0); // Wait until UDR is ready to be written to
		//Loop through the character array, writing a character to UDR at a time
		for(int i = 0; i < strlen(send_data); i++) {
			UDR0 = send_data[i];
			printf("Transmited: %c", (char)send_data[i]);
		}

		while((UCSR0A & (1<<RXC0)) == 0); // Wait until data has been received and it can be read from UDR
		if(buffer_offset < BUFFER_SIZE) {
			received_data[buffer_offset++] = UDR0; // Read the received data
		}
		printf("Received: %s", received_data);
	}
	
	PROCESS_END();
}

// ISR(USART0_RX_vect) {
// 	if(buffer_offset < BUFFER_SIZE) {
// 			received_data[buffer_offset++] = UDR0; // Read the received data
// 	}
// }
