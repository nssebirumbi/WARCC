/*
 * Copyright (c) 2015, Copyright Robert Olsson / Radio Sensors AB
 * All rights reserved.
 *
 *
 *
 * This file is part of the Contiki operating system.
 *
 *
 * Some code adopted from Robert Olsson <robert@herjulf.se> and Manee
 * 
 * 
 * @Authors:
 * Ongom Daniel <ongomdaniel9@gmail.com>,
 * Nicholas Henry Ssebirumbi <nssebirumbi@gmail.com>,
 * Nabwire Babra <nabwirebabra3@gmail.com>,
 * Nanjuki Saidat <saidatnanjuki@gmail.com>,
 * Rogers Ategeka <atrodgers77@gmail.com >
 *
 * The application reads sensor data, transmits it via broadbast, unicast .. using RIME
 */

#include "contiki.h"
#include "contiki-lib.h"

#include <stdio.h>
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "lib/fs/fat/diskio.h"
#include "lib/fs/fat/ff.h"
#include "net/rime/rime.h"
#include "netstack.h"

#include "dev/leds.h"
#include "dev/watchdog.h"
#include "dev/battery-sensor.h"
#include "dev/temp-sensor.h"
#include "dev/temp_mcu-sensor.h"
#include "dev/ds1307.h"
#include "dev/sht25.h"
#include "math.h"
#include "dev/mcp3424.h"
#include "dev/pulse-sensor.h"
#include "dev/ms5611.h"
#include "sys/etimer.h"
#include "dev/adc.h"
#include "dev/i2c.h"
#include "dev/powertrace.h"

#include "wimea-ict-ota.h"

#include "sys/energest.h"
#include "sys/compower.h"

#define NAME_LENGTH 12
#define TAGMASK_LENGTH 100

#define NO_SENSORS 16
#define MAX_BCAST_SIZE 120
#define DEF_TTL 0xF

uint16_t i2c_probed1; /* i2c devices we have probed */

uint8_t seqno = 0, len = 0;
char report[200];

static int power_save = 0;      /* Power-save false */
static int error_status = 0;

static struct timer t;

static char default_sensors[50] = " T_MCU V_MCU V_IN V_A1 V_A2 ";

static char current_interval[TAGMASK_LENGTH];
static char current_name[TAGMASK_LENGTH];
static char current_mask[TAGMASK_LENGTH];

static char new_node_name[TAGMASK_LENGTH];
static char new_interval[TAGMASK_LENGTH];
static char new_mask_added[TAGMASK_LENGTH];
static char new_mask_removed[TAGMASK_LENGTH];

static char error_invalid_operator[TAGMASK_LENGTH];
static char error_no_parameter[TAGMASK_LENGTH];
static char error_invalid_name[TAGMASK_LENGTH];
static char error_interval_command[TAGMASK_LENGTH];
static char error_unknown_command[TAGMASK_LENGTH];
static char error_incomplete_mask_command[TAGMASK_LENGTH];

static int I2C_SHT25_flag = 0;
static int pulses = 0;
static int I2C_MCP3424_flag;
static int I2C_MS5611_flag;

uint16_t EEMEM eemem_report_0_transmission_interval;

uint8_t EEMEM eemem_node_name[NAME_LENGTH];
uint8_t EEMEM eemem_adc1[NAME_LENGTH];
uint8_t EEMEM eemem_adc2[NAME_LENGTH];
uint8_t EEMEM eemem_tagmask[TAGMASK_LENGTH];
uint8_t EEMEM eemem_error_codes[TAGMASK_LENGTH];

uint8_t EEMEM eemem_report_0[TAGMASK_LENGTH];

uint16_t EEMEM eemem_report_flag;
uint16_t EEMEM eemem_name_flag;

uint16_t EEMEM eemem_I2C_SHT25_flag;
uint16_t EEMEM eemem_I2C_MCP3424_flag;
uint16_t EEMEM eemem_I2C_MS5611_flag;

uint16_t EEMEM eemem_tagmask_flag;

/*flags to check if eeprom has valid values*/

uint16_t EEMEM eemem_report_interval_flag;
uint16_t EEMEM eemem_report_1_interval_flag;
uint16_t EEMEM eemem_report_2_interval_flag;
uint16_t EEMEM eemem_report_3_interval_flag;

PROCESS(broadcast_data_process, "Broadcast sensor data");
PROCESS(init_process, "Initializes Process");
PROCESS(ota_trial_process, "This is OTA trial");
PROCESS(sensor_data_process, "Read sensor data");
PROCESS(windspeed_process, "Calcute wind speed");

unsigned char eui64_addr[8];

uint8_t rssi, lqi; // Received Signal Strength Indicator(RSSI), Link Quality Indicator(LQI)

static float wind_speed = 0.0;
static float max_windspeed = 0.0;

struct etimer et;
struct etimer et0;
struct etimer et2;
struct etimer et3;

uint8_t ttl = DEF_TTL;

struct broadcast_message {
	uint8_t head;
	uint8_t seqno;
	char buf[MAX_BCAST_SIZE + 20];
};


AUTOSTART_PROCESSES(&init_process, &broadcast_data_process, &ota_trial_process, &sensor_data_process, &windspeed_process);

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function broadcast_recv(): Receives the Incoming Commands Inform of Packet Buffers           *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	struct broadcast_message *msg;
	char *message_token;
	char *report_interval;
	char *report_command;
	char *parameter;
	char *operator_value;
	char *node_name;
	char *received_mac_address;
	char mac_address[17];
	uint8_t name[NAME_LENGTH];
	uint8_t report_mask[TAGMASK_LENGTH];
	uint16_t interval;
	char delimiter[] = " ";

	msg = packetbuf_dataptr();
	rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

	len += sprintf(report, "%s [ADDR=%-d.%-d SEQ=%-d TTL=%-u RSSI=%-u LQI=%-u]\n", msg->buf,
	from->u8[0], from->u8[1], msg->seqno, msg->head & 0xF, rssi, lqi);
	report[len++] = '\0';

	leds_on(LEDS_YELLOW);
	leds_on(LEDS_RED);
	_delay_ms(100);

	message_token = strtok(msg->buf, (const char*)delimiter);

	for(int i = 0, u = 0; i < sizeof(eui64_addr); i++) {
		u += snprintf(mac_address+u, sizeof(mac_address)-u, "%02x", eui64_addr[i]);
	}

	received_mac_address = (char *) malloc(17);
	strlcpy(received_mac_address, message_token+16, 17);

	// if(!strncmp(mac_address, received_mac_address, 17)) {
		cli();
		eeprom_read_block((void*)&name, (const void*)&eemem_node_name, NAME_LENGTH);
		eeprom_read_block((void*)&report_mask, (const void*)&eemem_report_0, TAGMASK_LENGTH);
		interval = eeprom_read_word(&eemem_report_0_transmission_interval);
		sei();
		
		report_command = (char *) malloc(MAX_BCAST_SIZE);
		strlcpy(report_command, message_token+42, strlen(message_token)+1); // Extract the exact Command that was sent.

		if (!strncmp(report_command, "ri", 2)) {
			if(strlen(report_command) == 2) {
				display_reporting_interval();
			}
			else if((strlen(report_command+3) > 0) && (!strncmp(report_command+3, "1", 1) || !strncmp(report_command+3, "2", 1) || !strncmp(report_command+3, "3", 1) || !strncmp(report_command+3, "4", 1) || !strncmp(report_command+3, "5", 1) || !strncmp(report_command+3, "6", 1))) {
				report_interval = (char *) malloc(4);
				strlcpy(report_interval, report_command+3, 4);
				change_reporting_interval(report_interval);

				snprintf(new_interval, 100, "FB-Report Interval Changed to: *%s*", report_interval);
				free(report_interval);

				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, new_interval);
			}
			else if((strlen(report_command+3) > 0) && (!strncmp(report_command+3, "current", 7))) {
				snprintf(current_interval, 255, "FB-Current Reporting Interval is *%d*", interval);
				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, current_interval);
			}
			else {
				printf("Incorrect Command *%s*, Try *ri <interval>*, to change the interval e.g. ri 10\n", report_command);
				snprintf(error_interval_command, 255, "FB-Incorrect Command *%s*, Try *ri <interval>* e.g. ri 10", report_command);
				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_interval_command);
			}
		}
		else if (!strncmp(report_command, "re", 2)) {
			if(strlen(report_command) == 2) {
				display_tagmask();
			}
			else if ((strlen(report_command+3) > 0) && (!strncmp(report_command+3, "current", 7))) {
				snprintf(current_mask, sizeof(current_mask), "FB-Current Report Mask: *%s*", report_mask);
				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, current_mask);
			}
			else if((strlen(report_command+3) > 0)){
				operator_value = (char*) malloc(2);
				strlcpy(operator_value, report_command+3, 2);

				if (strlen(report_command+5) > 0) {
					parameter = (char*) malloc(TAGMASK_LENGTH);
					strlcpy(parameter, report_command+5, TAGMASK_LENGTH);

					if (!strncmp(operator_value, "+", 1)) {
						add_to_report_mask(parameter);
						snprintf(new_mask_added, sizeof(new_mask_added), "FB-Parameters *%s* added to Report Mask", parameter);
						process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, new_mask_added);
					}
					else if(!strncmp(operator_value, "-", 1)) {
						remove_from_report_mask(parameter);
						snprintf(new_mask_removed, sizeof(new_mask_removed), "FB-Parameters *%s* removed from Report Mask", parameter);
						process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, new_mask_removed);
					}
					else {
						printf("Error, Invalid Operator in command: *%s*, Try *re + <parameter>* or *re - <parameter>*. e.g re + V_IN\n", report_command);
						snprintf(error_invalid_operator, sizeof(error_invalid_operator), "FB-Invalid Operator in command: *%s*, Try *re + <parameter>*", report_command);
						process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_invalid_operator);
					}
					free(parameter);
				}
				else {
					printf("Error, No Parameter Specified in Command: *%s*, Try *re + <parameter>*. e.g re + T\n", report_command);
					snprintf(error_no_parameter, sizeof(error_no_parameter), "FB-No Parameter Specified in Command: *%s*, Try *re + <parameter>*", report_command);
					process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_no_parameter);
				}
				free(operator_value);
			}
			else {
				printf("Invalid or Incomplete command: *%s*, Try *re + <parameter> to add Parameters, e.g re + P\n", report_command);
				snprintf(error_incomplete_mask_command, sizeof(error_incomplete_mask_command), "FB-Invalid or Incomplete command: *%s*, Try *re + <parameter>. e.g re + P", report_command);
				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_incomplete_mask_command);
			}
		}
		else if(!strncmp(report_command, "n", 1)) {
			if(strlen(report_command) == 1) {
				display_node_name();
			}
			else if((strlen(report_command+2) > 0)) {
				if (!strncmp(report_command+2, "current", 7)) {
					snprintf(current_name, sizeof(current_name), "FB-Current Node Name is: *%s*", name);
					process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, current_name);
				}
				else {
					node_name = (char*)malloc(7);
					strlcpy(node_name, report_command+2, 7);
					change_node_name(node_name);

					snprintf(new_node_name, sizeof(new_node_name), "FB-Node Name Changed to: *%s*", node_name);
					free(node_name);
					process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, new_node_name);
				}
			}
			else {
				snprintf(error_invalid_name, sizeof(error_invalid_name), "FB-Invalid or No name specified in command: *%s*, Try *name <node-name>*", report_command);
				process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_invalid_name);
			}
		}
		else if(!strncmp(report_command, "boot", 4)) {
			watchdog_reboot(); // Reboot the mote
		}
		else {
			printf("Sorry, Command **%s** is Not Recognized, or its not implemented...\n", report_command);
			snprintf(error_unknown_command, sizeof(error_unknown_command), "Error, Unknown Command: *%s*", report_command);
			process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, error_unknown_command);
		}
		free(report_command);
	// }

	pwr_pin_on();
	timer_set(&t, CLOCK_SECOND/100); //5ms pulse to wake up electron from sleep
	while(!timer_expired(&t));
	pwr_pin_off();

	printf("%s", report);
	leds_off(LEDS_RED);
	leds_off(LEDS_YELLOW);
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      PROCESS: init_process -> Initializes and Configures the flags.                               *@*
*@*             Starts the Broadcast SERVICES [Receive and Connect]                                   *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
PROCESS_THREAD(init_process, ev, data)
{
	power_save = 1;
	uint16_t tagmask_flag, report_flag;
	uint16_t report_interval_flag;

	char adc1[NAME_LENGTH] = "V_A1";
	char adc2[NAME_LENGTH] = "V_A2";
	int time_interval_1;

	I2C_SHT25_flag = 0;
	I2C_MCP3424_flag = 0 ;
	I2C_MS5611_flag = 0;
	power_save = 0;
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast));

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);
	NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, 26);

	cli();
	eeprom_update_word(&eemem_I2C_SHT25_flag, I2C_SHT25_flag);
	eeprom_update_word(&eemem_I2C_MCP3424_flag, I2C_MCP3424_flag);
	eeprom_update_word(&eemem_I2C_MS5611_flag, I2C_MS5611_flag);

	report_interval_flag = eeprom_read_word(&eemem_report_interval_flag);
	tagmask_flag = eeprom_read_word(&eemem_tagmask_flag);
	report_flag = eeprom_read_word(&eemem_report_flag);

	if (report_interval_flag != 1) {
		time_interval_1 = 60;
		eeprom_update_word(&eemem_report_0_transmission_interval, time_interval_1);
		report_interval_flag = 1;
		eeprom_update_word(&eemem_report_interval_flag, report_interval_flag);
	}

	if (tagmask_flag != 1) {
		tagmask_flag = 1;
		eeprom_update_word(&eemem_tagmask_flag, tagmask_flag);
		eeprom_update_block((const void *)&adc1, (void *)&eemem_adc1, NAME_LENGTH);
		eeprom_update_block((const void *)&adc2, (void *)&eemem_adc2, NAME_LENGTH);
	}

	if (report_flag != 1) {
		report_flag = 1;
		eeprom_update_word(&eemem_report_flag, report_flag);
		eeprom_update_block((const void *)&adc1, (void *)&eemem_adc1, NAME_LENGTH);
		eeprom_update_block((const void *)&adc2, (void *)&eemem_adc2, NAME_LENGTH);
	}
	set_default_report_mask();
	sei();
	i2c_at24mac_read((char *) &eui64_addr, 1);

	PROCESS_END();
}


PROCESS_THREAD(ota_trial_process, ev, data)
{
	PROCESS_BEGIN();

	printf("\n\t WIMEA-ICT-OTA Testing Process Working Fine: \t\n");

	PROCESS_END();
}

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      PROCESS: broadcast_data_process: Waits for a Synchronous EVENT and Broadcast                 *@*
*@*             the Data Received From the sender Process                                             *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
PROCESS_THREAD(broadcast_data_process, ev, data)
{
	char node[NAME_LENGTH];
	uint8_t node_name[NAME_LENGTH];
	uint8_t len, i = 0;
	static uint8_t seqno;
	struct broadcast_message msg;

	PROCESS_BEGIN();

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
		
		if(!strncmp((char*)data, "FB", 2)) {
			i += snprintf(msg.buf+i, strlen(data)+1, "%s", (char*)data);
			msg.buf[i++] = '\0';
			
			packetbuf_clear();
			packetbuf_copyfrom(&msg, i+2);
			broadcast_send(&broadcast);
			i += snprintf(msg.buf+i, 2, "\n\r");
		}
		else {
			cli();
			eeprom_read_block((void*)&node_name, (const void*)&eemem_node_name, NAME_LENGTH);
			sei();

			len = strlen(data);
			if (node_name > 0){
				strlcpy(node, (char*)node_name, NAME_LENGTH);
				i += snprintf(msg.buf+i, 30, "&: NAME=%s ", node);
			}
		
			/* Read out mote 64bit MAC address */
			len += 25;
			i += snprintf(msg.buf+i, len, "E64=%02x%02x%02x%02x%02x%02x%02x%02x %s", eui64_addr[0], eui64_addr[1], eui64_addr[2], eui64_addr[3], eui64_addr[4], eui64_addr[5], eui64_addr[6], eui64_addr[7], (char*)data);
		
			msg.buf[i++] = '\0'; // Null terminate report.
			printf("%s\n", msg.buf);
		
			msg.head = 1<<4;
			msg.head |= ttl;
			msg.seqno = seqno;

			packetbuf_clear();
			packetbuf_copyfrom(&msg, i+2);
			broadcast_send(&broadcast);
			seqno++;
			i += snprintf(msg.buf+i, 2, "\n\r"); // Append new line before data is buffered
		}
	}
	PROCESS_END();
}

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      PROCESS: sensor_data_process: Checks for sensor connection and reads the                     *@*
*@*             Sensor Data From the sensors connected                                                *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
PROCESS_THREAD(sensor_data_process, ev, data)
{
	PROCESS_BEGIN();
	// Activate all the required sensors
	SENSORS_ACTIVATE(temp_sensor);
	SENSORS_ACTIVATE(temp_mcu_sensor);
	SENSORS_ACTIVATE(battery_sensor);
	SENSORS_ACTIVATE(pulse_sensor);

	static int counter = 0;
	static int time_interval = 0;
	static int i, j;

	while(1) {
		time_interval = eeprom_read_word(&eemem_report_0_transmission_interval);

		if(time_interval != 0 && (adc_read_v_in() < 3.71)){
			time_interval = 120;
		}
		else{
			time_interval = eeprom_read_word(&eemem_report_0_transmission_interval);
		}
		etimer_set(&et0, CLOCK_SECOND * time_interval);
		PROCESS_WAIT_EVENT();
		//PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et0));
		if(etimer_expired(&et0)) {
			if(time_interval != 0){
				if(time_interval == 120){
					counter++;
					if(counter == 10){
						check_sensor_connection();
						NETSTACK_RADIO.on();
						read_sensor_values();
						NETSTACK_RADIO.off();
						counter = 0;
					}
				}
				else{
					check_sensor_connection();
					NETSTACK_RADIO.on();
					read_sensor_values();
					NETSTACK_RADIO.off();
					counter = 0;
				}
			}
			etimer_reset(&et0);
		}

		// for(i = 0; i < 2; i++) {   /* Loop over min and max rpc settings  */
		// 	NETSTACK_RADIO.off(); /* Radio off for rpc change */
		// 	if(i == 0){
		// 		rf230_set_rpc(0x0); /* Disbable all RPC features */
		// 	}
		// 	else{
		// 		rf230_set_rpc(0xFF); /* Enable all RPC features. Only XRFR2 radios */
		// 	}
		// 	/*  Loop over the different TX power settings 0-15  */
		// 	for(j = 15; j >= 0; j--) {
		// 		NETSTACK_RADIO.on();
		// 		rf230_set_txpower(j);
		// 		NETSTACK_RADIO.off();
		// 	}
		// 	if(i == 1){
		// 		time_interval = eeprom_read_word(&eemem_report_0_transmission_interval);

		// 		if(time_interval != 0 && (adc_read_v_in() < 3.71)){
		// 			time_interval = 120;
		// 		}
		// 		else{
		// 			time_interval = eeprom_read_word(&eemem_report_0_transmission_interval);
		// 		}
		// 		etimer_set(&et0, CLOCK_SECOND * time_interval);
				
		// 		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et0));
				
		// 		if(time_interval != 0){
		// 			if(time_interval == 120){
		// 				counter++;
		// 				if(counter == 10){
		// 					check_sensor_connection();
		// 					NETSTACK_RADIO.on();
		// 					read_sensor_values();
		// 					NETSTACK_RADIO.off();
		// 					counter = 0;
		// 				}
		// 			}
		// 			else{
		// 				check_sensor_connection();
		// 				NETSTACK_RADIO.on();
		// 				read_sensor_values();
		// 				NETSTACK_RADIO.off();
		// 				counter = 0;
		// 			}
		// 		}
		// 	}
		// }
	}
	// Deactive all the sensors
	SENSORS_DEACTIVATE(temp_sensor);
	SENSORS_DEACTIVATE(temp_mcu_sensor);
	SENSORS_DEACTIVATE(battery_sensor);
	SENSORS_DEACTIVATE(ds1307_sensor);
	SENSORS_DEACTIVATE(ms5611_sensor);
	SENSORS_DEACTIVATE(mcp3424_sensor);
	SENSORS_DEACTIVATE(sht25_sensor);
	SENSORS_DEACTIVATE(pulse_sensor);

	PROCESS_END();
}

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      PROCESS: windspeed_process: Calculate the windspeed depending on                             *@*
*@*                       Pulses received from the pulse sensors                                      *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
PROCESS_THREAD(windspeed_process, ev, data)
{
	PROCESS_BEGIN();

	uint8_t node_name[NAME_LENGTH];
	char *type;
	int index = -1;
	while(1) {
		eeprom_read_block((void*)&node_name, (const void*)&eemem_node_name, NAME_LENGTH);
		type = (char *)node_name;
		const char *ptr = strchr(type, (int)"10");

		if(ptr) {
			index = ptr - type;
		}
		if(index <= -1){
			// To allow rainguage capture data
			etimer_set(&et2, CLOCK_SECOND * 100);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et2));
		}
		else{
			etimer_set(&et2, CLOCK_SECOND * 1);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et2));

			pulses = pulse_sensor.value(0);
			wind_speed = 2.5 * pulses;

			if(wind_speed > max_windspeed){
				max_windspeed = wind_speed;
			}
		}
	}
	PROCESS_END();
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function display_reporting_interval(): Takes in No Parameters                                *@*
*@*             Displays the Reporting Interval of the Current Report.                                *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
display_reporting_interval(void)
{
	uint16_t report_interval;
	cli();
	report_interval = eeprom_read_word(&eemem_report_0_transmission_interval);
	sei();
	printf("Current Reporting Interval is := %d\n", report_interval);
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function current_reporting_interval(): Takes in No Parameters                                *@*
*@*             Returns the Reporting Interval of the Current Report.                                 *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
uint16_t
current_reporting_interval(void)
{
	uint16_t report_interval;
	cli();
	report_interval = eeprom_read_word(&eemem_report_0_transmission_interval);
	sei();
	
	return report_interval;
}

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function change_reporting_interval(): Takes in Parameters <report_interval>                  *@*
*@*             Changes the Reporting Interval to a given Interval value.                             *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
change_reporting_interval(char *value)
{
	// Check if value is an integer
	int interval = atoi(value);
	if (interval == 0 && !strncmp(value, "0", 1)) {
		printf("Invalid Report Interval: %s!, Please enter an interger for period.\n", value);
		return;
	}
	
	cli();
	eeprom_update_word(&eemem_report_0_transmission_interval, interval);
	sei();
	printf("Reporting Interval has been changed to %ds\n", interval);
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function add_to_report_mask(): Takes in Parameters <parameter>                               *@*
*@*                       Adds a given Parameter to the  Report Mask                                  *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
add_to_report_mask(char *parameter)
{
	int parameter_length, size;
	int m = 0, d = 1, correct = 0;

	char *report_mask_to_add = (char*) malloc(TAGMASK_LENGTH);
	char *parameter_value = (char*) malloc(TAGMASK_LENGTH); //store mask from the user

	char *sensors[17] = {"ADC_1","ADC_2","ADC_3","ADC_4","RH","P","T","T1","V_A1","V_A2","V_IN","T_MCU","V_MCU","INTR","P0_LST60","WDSPD","MAXWSD"}; //array of available sensors

	char *sensorsadd[17];
	uint8_t report_add[TAGMASK_LENGTH]; // Read and Store the Current  Report Mask From eeprom
	cli();
	eeprom_read_block((void*)&report_add, (const void*)&eemem_report_0, TAGMASK_LENGTH);
	sei();

	char *split_report_Mask_to_add, *split_report_parameter; //store the mask with sanitized values that we are going to write to eeprom
	strlcpy(report_mask_to_add, (char*)report_add, TAGMASK_LENGTH); //
	split_report_Mask_to_add = strtok (report_mask_to_add, " "); // Split the Report Parameters stored in the Memory with commas

	while( split_report_Mask_to_add != NULL ) {
		split_report_Mask_to_add = trim(split_report_Mask_to_add);
		sensorsadd[m] = split_report_Mask_to_add;
		m++;
		split_report_Mask_to_add = strtok(NULL, " ");
	}

	strlcpy(parameter_value, (char*)parameter, TAGMASK_LENGTH);
	split_report_parameter = strtok (parameter_value, " "); // Split the New Parameters to be added with space

	while( split_report_parameter != NULL ) {
		d = 1; correct = 0;
		split_report_parameter = trim(split_report_parameter);
		parameter_length = strlen(split_report_parameter);
		for (int v = 0; v < m; v++){
			if (!strncmp(sensorsadd[v], split_report_parameter, parameter_length)){
				d = 0;
			}
		}
		for(volatile int x = 0; x < 18; x++){
			if (!strncmp(sensors[x], split_report_parameter, parameter_length)){
				correct = 1;
			}
		}
		if(d == 1 && correct == 1){
			size = strlen(split_report_parameter);
			strncat((char*)report_add, " ", 1);
			strncat((char*)report_add, split_report_parameter, size);
		}
		split_report_parameter = strtok(NULL, " ");
	}
	if (strlen((char*)report_add) > 0) { // Check if  Report Mask is not Empty after validation
		cli();
		eeprom_update_block((const void *)&report_add, (void *)&eemem_report_0, TAGMASK_LENGTH);
		sei();
		printf("Parameter **%s** added successfully to  Report Mask\n", (char*)parameter);
	}
	free(parameter_value);
	free(report_mask_to_add);
}

/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function remove_from_report_mask(): Takes in Parameters <parameter>                          *@*
*@*                 Removes a given Parameter from the Current Report Mask                            *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
remove_from_report_mask(char* parameter)
{
	int parameter_length, size;
	int m = 0, d = 1, correct = 0, k = 0;
	char *tagmaskremove = (char*) malloc(TAGMASK_LENGTH);
	char *parameter_value = (char*) malloc(TAGMASK_LENGTH); // Store  Report Mask from the user
	char *sensors[17] = {"ADC_1","ADC_2","ADC_3","ADC_4","RH","P","T","T1","V_A1","V_A2","V_IN","T_MCU","V_MCU","INTR","P0_LST60","WDSPD","MAXWSD"}; // Array of available sensors
	char *sensorsremove[17];
	uint8_t report_remove[TAGMASK_LENGTH]; // Store report parameter from Memory

	cli();
	eeprom_read_block((void*)&report_remove, (const void*)&eemem_report_0, TAGMASK_LENGTH);
	sei();

	char *split_tagmaskremove, *split_parameterremove, save_tagmaskremove[TAGMASK_LENGTH] = {}; //store the mask with sanitized values that we are going to write to eeprom
	strlcpy(parameter_value, (char*)parameter, TAGMASK_LENGTH);
	split_parameterremove = strtok (parameter_value, " "); // Split the  Report Mask from user with space

	while( split_parameterremove != NULL ) {
		split_parameterremove = trim(split_parameterremove);
		sensorsremove[m] =  split_parameterremove;
		m++;
		split_parameterremove = strtok(NULL, " ");
	}

	strlcpy(tagmaskremove, (char*)report_remove, TAGMASK_LENGTH);
	split_tagmaskremove = strtok (tagmaskremove, " "); // Split the  Report Mask from eeprom with commas

	while( split_tagmaskremove != NULL ) {
		d = 1; correct = 0;
		split_tagmaskremove = trim(split_tagmaskremove);
		parameter_length = strlen(split_tagmaskremove);
		for (int v = 0; v < m; v++){
			if (!strncmp(sensorsremove[v], split_tagmaskremove, parameter_length)){
				d = 0;
			}
		}
		for(volatile int x = 0; x < 18; x++){
			if (!strncmp(sensors[x], split_tagmaskremove, parameter_length)){
				correct = 1;
			}
		}
		if(d == 1 && correct == 1){
			if(k == 0){
				size = strlen(split_tagmaskremove);
				strlcpy((char*)save_tagmaskremove, split_tagmaskremove, size+1);
				k++;
			}
			else{
				size = strlen(split_tagmaskremove);
				strncat((char*)save_tagmaskremove, " ", 1);
				strncat((char*)save_tagmaskremove, split_tagmaskremove, size);
				k++;
			}
		}
		split_tagmaskremove = strtok(NULL, " ");
	}
	cli();
	if (strlen((char*)save_tagmaskremove) > 0) {
		eeprom_update_block((const void *)&save_tagmaskremove, (void *)&eemem_report_0, TAGMASK_LENGTH);
		printf("Parameter *%s* removed successfully from  Report Mask\n", parameter);
	}
	sei();

	free(tagmaskremove);
	free(parameter_value);
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function display_tagmask(): Displays the  Report Mask for all the Reports                    *@*
*@*                 Displays the  Report Mask Stored in eeprom                                        *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
display_tagmask(void)
{
	uint8_t report[TAGMASK_LENGTH];

	cli();
	eeprom_read_block((void*)&report, (const void*)&eemem_report_0, TAGMASK_LENGTH);
	sei();

	printf("Report Parameters := %s\n", (char*)report);
	printf("Possible  Report Mask parameters := ADC_1, ADC_2, ADC_3, ADC_4, RH, P, T, T1, V_A1,V_A2, V_IN, T_MCU, V_MCU, INTR, P0_LST60, WDSPD, MAXWSD\n");
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*      Function set_default_report_mask(): Sets Default Report Mask for Report 0                    *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
set_default_report_mask(void)
{
	cli();
	eeprom_update_block((const void *)&default_sensors, (void *)&eemem_report_0, TAGMASK_LENGTH);
	sei();
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*     Function auto_set_report_mask(): Takes in Parameter <mask_value, size, status>                *@*
*@*                 Automatically adds the Parameter values for the connected sensors                 *@*
*@*                                     to the report mask                                            *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
auto_set_report_mask(char *value2[], int size1, int sensor_status)
{

	int i, size, m = 0, d = 1, k = 0, j = 0;
	char *tagmask2 = (char*) malloc(TAGMASK_LENGTH); //store mask from the user

	char *sensors[17] = {"ADC_1", "ADC_2", "ADC_3", "ADC_4", "RH", "P", "T", "T1", "V_A1", "V_A2", "V_IN", "T_MCU", "V_MCU", "INTR", "P0_LST60", "WDSPD", "MAXWSD"}; //array of available sens3

	char *sensors3[17];
	uint8_t report_a2[TAGMASK_LENGTH];

	cli();
	eeprom_read_block((void*)&report_a2, (const void*)&eemem_report_0, TAGMASK_LENGTH);
	sei();

	//tore the mask with sanitized values that we are going to write to eeprom
	char *split_tagmask2, save_tagmask2[TAGMASK_LENGTH] = {};
	strlcpy(tagmask2, (char*)report_a2, TAGMASK_LENGTH);
	// Split the string with commas
	split_tagmask2 = strtok (tagmask2, " ");

	while( split_tagmask2 != NULL ) {
		split_tagmask2 = trim(split_tagmask2);
		sensors3[m] = split_tagmask2;
		m++;
		split_tagmask2 = strtok(NULL, " ");
	}
	if(sensor_status == 1){
		for (i = 0; i < size1; i++){
			d = 1;
			for (int v = 0; v < m; v++){
				size = strlen(value2[i]);
				if (!strncmp(sensors3[v], value2[i], size)){
					d = 0;
				}
			}
			if(d == 1 && sensor_status == 1){
				size = strlen(value2[i]);
				strncat((char*)report_a2, " ", 1);
				strncat((char*)report_a2, value2[i], size);
			}
		}
		if (strlen((char*)report_a2) > 0) {// check if tagmask is not empty after validation
			cli();
			eeprom_update_block((const void *)&report_a2, (void *)&eemem_report_0, TAGMASK_LENGTH);
			sei();
		}
	}
	if(sensor_status == 0){
		for (i = 0; i < m; i++){
			d = 1; j = 0;
			for (int v = 0; v < size1; v++){
				size = strlen(sensors3[i]);
				if (!strncmp(sensors3[i], value2[v], size)){
					d = 0;
					if(!strncmp(sensors3[i],"T", 1)&& !strncmp(value2[v], "T_MCU", 5)){
						d = 1;
					}
				}
			}
			for (volatile int h = 0; h < 18; h++){
				size = strlen(sensors[j]);
				if (!strncmp(sensors3[i], sensors[h], size)){
					j = 1;
				}
			}
			if(d == 1 && sensor_status == 0 && j == 1 ){
				if(k == 0){
					size = strlen(sensors3[i]);
					strlcpy((char*)save_tagmask2, sensors3[i] , size+1);
					k++;
				}
				else{
					size = strlen(sensors3[i]);
					strncat((char*)save_tagmask2, " ", 1);
					strncat((char*)save_tagmask2, sensors3[i], size);
				}
				k++;
			}
		}
		if (strlen((char*)save_tagmask2) > 0) {
			cli();
			eeprom_update_block((const void *)&save_tagmask2, (void *)&eemem_report_0, TAGMASK_LENGTH);
			sei();
		}
	}
	free(tagmask2);
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*      Function check_sensor_connection(): Takes in no Parameter                                    *@*
*@*                 Check for sensor connections. It has a plug & play capability                     *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
check_sensor_connection(void)
{
	i2c_probed1 = i2c_probe();
	I2C_SHT25_flag = 0;
	I2C_MCP3424_flag = 0;
	I2C_MS5611_flag = 0;

	if( i2c_probed1 & I2C_SHT25) {
		SENSORS_ACTIVATE(sht25_sensor);
		char *mask[]= {"RH","T"};
		auto_set_report_mask(mask, 2, 1);
		I2C_SHT25_flag = 1;
	}
	else if(!(i2c_probed1 & I2C_SHT25)){
		char *mask[]= {"RH","T"};
		auto_set_report_mask(mask, 2, 0);
		I2C_SHT25_flag = 0;
	}

	if( i2c_probed1 & I2C_MCP3424 ) {
		SENSORS_ACTIVATE(mcp3424_sensor);
		char *mask[]= {"ADC_1","ADC_2","ADC_3","ADC_4"};
		auto_set_report_mask(mask, 4, 1);
		I2C_MCP3424_flag = 1;
	}
	else if(!(i2c_probed1 & I2C_MCP3424)){
		char *mask[]= {"ADC_1", "ADC_2", "ADC_3", "ADC_4"};
		auto_set_report_mask(mask, 4, 0);
		I2C_MCP3424_flag = 0;
	}

	if(i2c_probed1 & I2C_MS5611_ADDR){
		SENSORS_ACTIVATE(ms5611_sensor);
		char *mask[]= {"P"};
		auto_set_report_mask(mask, 1, 1);
		I2C_MS5611_flag = 1;
	}
	else if(!(i2c_probed1 & I2C_MS5611_ADDR)){
		char *mask[]= {"P"};
		auto_set_report_mask(mask, 1, 0);
		I2C_MS5611_flag = 0;
	}
	if( i2c_probed1 & I2C_DS1307 ) {
		DS1307_init();
		SENSORS_ACTIVATE(ds1307_sensor);
	}

	cli();

	if((eeprom_read_word(&eemem_I2C_SHT25_flag) != I2C_SHT25_flag) && (I2C_SHT25_flag == 0)){
		printf( "\nAlert \t******** I2C_SHT25 SENSOR Disconnected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_SHT25_flag, I2C_SHT25_flag);
		error_log("#E_Dc(SHT25)");
	}
	else if((eeprom_read_word(&eemem_I2C_SHT25_flag) != I2C_SHT25_flag )&& (I2C_SHT25_flag == 1)){
		printf( "\nAlert \t******** I2C_SHT25 SENSOR Connected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_SHT25_flag, I2C_SHT25_flag);
	}

	if((eeprom_read_word(&eemem_I2C_MS5611_flag) != I2C_MS5611_flag )&& (I2C_MS5611_flag == 0)){
		printf( "\nAlert \t******** I2C_MS5611 SENSOR Disconnected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_MS5611_flag, I2C_MS5611_flag);
		error_log("#E_Dc(MS5611)");
	}
	else if((eeprom_read_word(&eemem_I2C_MS5611_flag) != I2C_MS5611_flag )&& (I2C_MS5611_flag == 1)){
		printf( "\nAlert \t******** I2C_MS5611 SENSOR Connected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_MS5611_flag, I2C_MS5611_flag);
	}

	if((eeprom_read_word(&eemem_I2C_MCP3424_flag) != I2C_MCP3424_flag) && (I2C_MCP3424_flag ==0)){
		printf( "\nAlert \t******** I2C_MCP3424 SENSOR Disconnected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_MCP3424_flag, I2C_MCP3424_flag);
		error_log("#E_Dc(MCP3424)");
	}
	else if((eeprom_read_word(&eemem_I2C_MCP3424_flag) != I2C_MCP3424_flag) && (I2C_MCP3424_flag ==1)){
		printf( "\nAlert \t******** I2C_MCP3424 SENSOR Connected ***********\n\n" );
		eeprom_update_word(&eemem_I2C_MCP3424_flag, I2C_MCP3424_flag);
	}

	sei();

}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*     Function read_sensor_values(): Takes in no Parameter                                          *@*
*@*                 Returns the Report mask data from various sensors                                 *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
read_sensor_values(void)
{
	static int adc1 = 0, adc2 = 0, adc3 = 0, adc4 = 0;
	char result[TAGMASK_LENGTH + 2], *sensors;
	uint8_t tagmask0[TAGMASK_LENGTH], error[TAGMASK_LENGTH], i = 0;

	cli();
	eeprom_read_block((void*)&tagmask0, (const void*)&eemem_report_0, TAGMASK_LENGTH);
	sei();

	sensors = strtok((char*)tagmask0, " ");
	static int P0_LST = 0;
	printf("Inside the Read Sensor Values Function: \n");
	while (sensors != NULL){
		if(adc_read_v_in() < 2.70){
			if (!strncmp(trim(sensors), "T_MCU", 5)) {
				i += snprintf(result+i, 12, " T_MCU=%-4.1f", ((double) temp_mcu_sensor.value(0)/10.));
			}
			else if (!strncmp(trim(sensors), "V_MCU", 5)) {
				i += snprintf(result+i, 11, " V_MCU=%-3.1f", ((double) battery_sensor.value(0))/1000.);
			}
			else if (!strncmp(trim(sensors), "V_IN", 4)) {
				i += snprintf(result+i, 11, " V_IN=%-4.2f", adc_read_v_in());
				error_log("L-PWR");
			}
		}
		else{
			if (!strncmp(trim(sensors), "T_MCU", 5)) {
				i += snprintf(result+i, 12, " T_MCU=%-4.1f", ((double) temp_mcu_sensor.value(0)/10.));
			}
			else if (!strncmp(trim(sensors), "V_MCU", 5)) {
				i += snprintf(result+i, 11, " V_MCU=%-3.1f", ((double) battery_sensor.value(0))/1000.);
			}
			else if (!strncmp(trim(sensors), "V_IN", 4)) {
				i += snprintf(result+i, 11, " V_IN=%-4.2f", adc_read_v_in());
			}
			else if (!strncmp(trim(sensors), "V_A1", 4)) {
				if(adc_read_a1() < 0.15 || adc_read_a1() > 5.00){
					error_log("#E_wv(V_A1)");
				}
				else{
					i += snprintf(result+i, 15, " V_A1=%.2f", adc_read_a1());
				}
			}
			else if (!strncmp(trim(sensors), "V_A2", 4)) {
				if(adc_read_a1() < 0.15 || adc_read_a1() > 5.00){
					error_log("#E_wv(V_A2)");
				}
				else{
					i += snprintf(result+i, 13, " V_A2=%.2f", adc_read_a2());
				}
			}
			else if (!strncmp(trim(sensors), "T1", 2)) {
				if((temp_sensor.value(0) * 1.0/100) < -55.0 || (temp_sensor.value(0) * 1.0/100) > 125.0){
					error_log("#E_wv(T1)");
				}
				else{
					i += snprintf(result+i, 10, " T1=%-5.2f", (double)(temp_sensor.value(0)*1.0/100));
				}
			}
			else if (!strncmp(trim(sensors), "P", 1)) {   //pressure
				if(i2c_probed1 & I2C_MS5611_ADDR){
					if(ms5611_sensor.value(0) && missing_p_value() == 0){
						if(ms5611_sensor.value(0) < 10.0 || ms5611_sensor.value(0) > 1200.0){
							error_log("#E_wv(P)");
						}
						else{
							i += snprintf(result+i, 12, " P=%d", ms5611_sensor.value(0));
						}
					}else{
						error_log("#E_mv(ms5611)");
					}
				}
			}
			else if (!strncmp(trim(sensors), "T", 1) ) {//temperature
				if( i2c_probed1 & I2C_SHT25){
					if(sht25_sensor.value(0) && missing_t_value() == 0){
						if((sht25_sensor.value(0)/10.0) < -40 || (sht25_sensor.value(0)/10.0) > 125){
							error_log("#E_wv(T)");
						}
						else{
							i += snprintf(result+i, 9, " T=%.2f", (float) sht25_sensor.value(0)/10.0);
						}
					}else{
						error_log("#E_mv(sht25)");
					}
				}
			}
			else if (!strncmp(trim(sensors), "RH", 2)) {//humidity
				if( i2c_probed1 & I2C_SHT25) {
					if(sht25_sensor.value(1) && missing_t_value() == 0){
						if((sht25_sensor.value(1)/10.0) < 0.0 || (sht25_sensor.value(1)/10.0) > 100){
							error_log("#E_wv(RH)");
						}else{
							i += snprintf(result+i, 10, " RH=%.2f", (float) sht25_sensor.value(1)/10.0);
						}
					}else{
						error_log("#E_mv(sht25)");
					}
				}
			}
			else if (!strncmp(trim(sensors), "INTR", 4) ) {//int pin eg. rain gauge and anenometer
				int pulse = pulse_sensor.value(0);
				i += snprintf(result+i, 15, " P0_LST60=%d ", pulse);
				P0_LST = P0_LST + pulse;
				i += snprintf(result+i, 15, " P0_LST=%d", P0_LST);

				if(P0_LST >= 1440){
					P0_LST = 0;
				}
			}
			else if (!strncmp(trim(sensors), "WDSPD", 5)) {
				i += snprintf(result+i, 15, " windSpeed=%.2f ", wind_speed);
			}
			else if (!strncmp(trim(sensors), "MAXWSD", 5)) {
				i += snprintf(result+i, 15, " mxwdspd=%.2f ", max_windspeed);
				max_windspeed = 0.0;
			}
			else if (!strncmp(trim(sensors), "ADC_1", 5)) {
				if( i2c_probed1 & I2C_MCP3424 ){
					if(mcp3424_sensor.value(0) && (missing_adc_value() == 0)){

						i += snprintf(result+i, 19, " ADC_1=%.6f ", (float) mcp3424_sensor.value(0)/1000000.0);
						adc1 = 1;
					}else{
						adc1 = 0;
					}
				}
			}
			else if (!strncmp(trim(sensors), "ADC_2", 5)) {
				if( i2c_probed1 & I2C_MCP3424 ){
					if(mcp3424_sensor.value(1) && missing_adc_value() == 0 && adc1 == 0){

						i += snprintf(result+i, 14, " ADC_2=%.4f", (float) mcp3424_sensor.value(1)/1000000.000);
						adc2 = 1;
					}else{
						adc2 = 0;
					}
				}
			}
			else if (!strncmp(trim(sensors), "ADC_3", 5)) {
				if( i2c_probed1 & I2C_MCP3424 ){
					if(mcp3424_sensor.value(2) && missing_adc_value() == 0 && adc1 == 0 && adc2 == 0){
						i += snprintf(result+i, 14, " ADC_3=%.4f", (float) mcp3424_sensor.value(2)/1000000.000);
						adc3 = 1;
					}else{
						adc3 = 0;
					}
				}
			}
			else if (!strncmp(trim(sensors), "ADC_4", 5)) {
				if( i2c_probed1 & I2C_MCP3424 ){
					if(mcp3424_sensor.value(3) && missing_adc_value() == 0 && adc1 == 0 && adc2 == 0 && adc3 == 0){
						i += snprintf(result+i, 14, " ADC_4=%.4f", (float) mcp3424_sensor.value(3)/1000000.000);
						adc4 = 1;
					}else{
						adc4 = 0;
						if(adc1 == 0 && adc2 == 0 && adc3 == 0 && adc4 == 0){
							error_log("#E_mv(mcp3424)");
						}
					}
				}
			}
		}
		if(error_status == 1){

			cli();
			eeprom_read_block((void*)&error, (const void*)&eemem_error_codes, TAGMASK_LENGTH);
			sei();

			i += snprintf(result+i, 16, " %s", error);
			error_status = 0;

		}
		// If the report is greater than 45bytes, send the current result, reset i and result
		if(i > 100) {
			// Null terminate result before sending
			result[i++] = '\0';
			// Send an event to broadcast process once data is ready
			process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, result);
			i = 0;
			result[0] = '\0';
		}
		sensors = strtok(NULL, " ");
	}
	// Null terminate result before sending
	result[i++] = '\0';
	// Send an event to broadcast process once data is ready
	process_post_synch(&broadcast_data_process, PROCESS_EVENT_CONTINUE, result);
}



/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*              Function trim(): Takes in Parameter <string> Strips Off WhiteSpaces                  *@*
*@*                      Trim whitespaces before and after a string                                   *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
char
*trim(char * str)
{
	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	if(*str == 0)  // All spaces?
	return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return str;
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*              Function error_log(): Takes in Parameter <error_message>                             *@*
*@*                      Logs the Error Message  onto the report                                      *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
error_log(char *message)
{
	char savelog[TAGMASK_LENGTH];
	strlcpy(savelog, message, TAGMASK_LENGTH);
	cli();
	eeprom_update_block((const void *)&savelog, (void *)&eemem_error_codes, TAGMASK_LENGTH);
	sei();
	error_status = 1;

}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*              Function change_node_name(): Takes in Parameter <name>                               *@*
*@*                      Changes the name of the node to <name>                                       *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
change_node_name(char *name)
{
	uint16_t name_flag = 1;
	char new_name[NAME_LENGTH];
	uint8_t node_name[NAME_LENGTH];
	strlcpy(new_name, name, NAME_LENGTH);

	cli();
    eeprom_update_block((const void *)&new_name, (void *)&eemem_node_name, NAME_LENGTH);
    eeprom_update_word(&eemem_name_flag, name_flag);
	sei();

	eeprom_read_block((void*)&node_name, (const void*)&eemem_node_name, NAME_LENGTH);
	printf("Current Node name changed to: %s\n", node_name);
}


/*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*                 Function display_node_name(): Takes in No Parameter                               *@*
*@*                      Display the current name of the Node                                         *@*
*@*                                                                                                   *@*
*@*                                                                                                   *@*
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
*/
void
display_node_name(void)
{
	uint16_t name_flag;
	uint8_t node_name[NAME_LENGTH];
	cli();
	name_flag = eeprom_read_word(&eemem_name_flag);
	if (name_flag != 1) {

		printf("Current Node's name not set\n");

	} else {
		eeprom_read_block((void*)&node_name, (const void*)&eemem_node_name, NAME_LENGTH);

		printf("Current Node's name is = %s\n", (char *)node_name);

	}
	sei();
}
