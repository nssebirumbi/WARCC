/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#define F_CPU 16000000UL

#include "usart-ota.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "contiki-conf.h"
#include "contiki.h"

#define RXC _BV(RXC0)
#define D_UDR0   UDR0
#define D_UDRE0M _BV(UDRE0)
#define D_UBRR0H UBRR0H
#define D_UBRR0L UBRR0L
#define D_UCSR0A UCSR0A
#define D_UCSR0B UCSR0B
#define D_UCSR0C UCSR0C
#define D_USART0_RX_vect USART0_RX_vect
#define D_USART0_TX_vect USART0_TX_vect

/* Insert a carriage return after a line feed. This is the default. */
#ifndef ADD_CARRIAGE_RETURN_AFTER_NEWLINE
#define ADD_CARRIAGE_RETURN_AFTER_NEWLINE 1
#endif

int (* usart_input_handler)(unsigned char);

ISR(D_USART0_RX_vect)
{
  unsigned char temp = D_UDR0;

  if (usart_input_handler != NULL) usart_input_handler(temp);
}

/*--------------------------------------------------------------------------*/
static int USART_stdout_putchar(char c, FILE *stream);

static uint8_t stdout_USART_port = RS232_PORT_0;

static FILE USART_stdout = FDEV_SETUP_STREAM(USART_stdout_putchar, NULL, _FDEV_SETUP_WRITE);

/*---------------------------------------------------------------------------*/
void
USART_init(uint8_t bd)
{
  D_UBRR0H = (uint8_t)(bd>>8);
  D_UBRR0L = (uint8_t)bd;

  // D_UCSR0B =  USART_RECEIVER_ENABLE | USART_TRANSMITTER_ENABLE;

  D_UCSR0B =  USART_RECEIVER_ENABLE | USART_TRANSMITTER_ENABLE | USART_INTERRUPT_RX_COMPLETE;

  D_UCSR0C = USART_MODE_ASYNC | USART_DATA_BITS_8 | USART_PARITY_NONE | USART_STOP_BITS_1;
  sei();
  stdout = &USART_stdout;
  usart_input_handler = NULL;
}

/*---------------------------------------------------------------------------*/
void 
USART_send(unsigned char c)
{
  while (!(D_UCSR0A & D_UDRE0M));
  D_UDR0 = c;
}
/*---------------------------------------------------------------------------*/
void
USART_send_string(char *str)
{
  for(size_t i = 0; i < strlen(str); i++)
  {
    USART_send(str[i]);
  }
}
/*---------------------------------------------------------------------------*/
unsigned char 
USART_read()
{
  while (!(D_UCSR0A & RXC));
  return D_UDR0;
}
/*---------------------------------------------------------------------------*/
void
USART_set_input(int (* f)(unsigned char))
{
  usart_input_handler = f;
}

/*---------------------------------------------------------------------------*/
void
USART_print(char *buf)
{
  while(*buf) {
    USART_send(*buf++);
    #if ADD_CARRIAGE_RETURN_AFTER_NEWLINE
      if(*buf == '\n') {
        // USART_send(*buf++);
        USART_send('\r');
      }
	    if(*buf == '\r') {
        buf++;
      } 
      else {
        USART_send(*buf++);
      }
    #else
      USART_send(*buf++);
    #endif
  }
}

#if USART_PRINTF_BUFFER_LENGTH
/*---------------------------------------------------------------------------*/
void
USART_printf(uint8_t port, const char *fmt, ...)
{*
  va_list ap;
  static char buf[USART_PRINTF_BUFFER_LENGTH];

  va_start (ap, fmt);
  vsnprintf (buf, USART_PRINTF_BUFFER_LENGTH, fmt, ap);
  va_end(ap);

  USART_print (port, buf);
}
#endif

/*---------------------------------------------------------------------------*/
int USART_stdout_putchar(char c, FILE *stream)
{
  #if ADD_CARRIAGE_RETURN_AFTER_NEWLINE
    if(c == '\n') {
      // USART_send(c);
      USART_send('\r');
    }
    if(c != '\r') {
      USART_send(c);
    }
  #else
    USART_send(c);
  #endif
  return 0;
}

/*---------------------------------------------------------------------------*/
void USART_redirect_stdout(uint8_t port)
{
  stdout_USART_port = port;
  stdout = &USART_stdout;
}

