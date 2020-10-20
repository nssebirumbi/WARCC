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
 * Author:   Adam Dunkels <adam@sics.se>
 *           Simon Barner <barner@in.tum.de>
 *
 */

#ifndef USART_H_
#define USART_H_

#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "usart_atmega256rfr2.h"

/**
 * \brief      Initialize the USART module
 *
 *             This function is called from the boot up code to
 *             initalize the USART module.
 * \param bd   The baud rate of the connection.
 */
void
USART_init(uint8_t bd);

/**
 * \brief      Set an input handler for incoming USART data
 * \param f    A pointer to a byte input handler
 *
 *             This function sets the input handler for incoming USART
 *             data. The input handler function is called for every
 *             incoming data byte. The function is called from the
 *             USART interrupt handler, so care must be taken when
 *             implementing the input handler to avoid race
 *             conditions.
 *
 *             The return value of the input handler affects the sleep
 *             mode of the CPU: if the input handler returns non-zero
 *             (true), the CPU is awakened to let other processing
 *             take place. If the input handler returns zero, the CPU
 *             is kept sleeping.
 */
void
USART_set_input(int (* f)(unsigned char));


/**
 * \brief      Print a text string from program memory on USART
 * \param buf  A pointer to the string that is to be printed
 *
 *             This function prints a string from program memory to
 *             USART. The string must be terminated by a null
 *             byte. The USART module must be correctly initalized and
 *             configured for this function to work.
 */
void
USART_print(char *buf);

/**
 * \brief      Print a formated string on USART
 * \param fmt  The format string that is used to construct the string
 *             from a variable number of arguments.
 *
 *             This function prints a formated string to USART. Note
 *             that this function used snprintf internally and thus cuts
 *             the resulting string after USART_PRINTF_BUFFER_LENGTH - 1
 *             bytes. You can override this buffer lenght with the
 *             USART_CONF_PRINTF_BUFFER_LENGTH define. The USART module
 *             must becorrectly initalized and configured for this
 *             function to work.
 */
void
USART_printf(const char *fmt);

/**
 * \brief      Print a character on USART
 * \param c    The character to be printed
 *
 *             This function prints a character to USART. The USART
 *             module must be correctly initalized and configured for
 *             this function to work.
 */
void
USART_send(unsigned char c);

/**
 * \brief      Redirects stdout to a given USART port
 *
 *             This function redirects the stdout channel to a given
 *             USART port. Note that this modfies the global variable
 *             stdout. If you want to restore the previous behaviour, it
 *             is your responsibility to backup to old value. The USART
 *             module must be correctly initalized and configured for
 *             the redirection to work.
 */
void
USART_redirect_stdout(uint8_t port);

void
USART_send_string(char *str);

void
execute(char *command);

unsigned char
USART_read();

#endif /* USART_H_ */
