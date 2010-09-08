/*****************************************************************************
 *   Copyright (C) 2010 by Christian Groeger                                 *
 *   code@proquari.at                                                        *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *                                                                           *
 *****************************************************************************/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>
#include "can.h"

#include "uart.h"

#define LED1_INIT()	PORTB&=~(1<<PB1)
#define LED1_ON()	DDRB|=(1<<PB1)
#define LED1_OFF()	DDRB&=~(1<<PB1)

#define LED2_INIT()	DDRC|=(1<<PC0)
#define LED2_ON()	PORTC|=(1<<PC0)
#define LED2_OFF()	PORTC&=~(1<<PC0)
#define LED2_TOGGLE()	PORTC ^= (1<<PC0)


#define BUFFER_FULL		'b'
#define SEND_ERROR		'e'
#define MALFORMED_MSG	'm'

/* If you want to receive both 11 and 29 bit identifiers, set your filters
 * and masks as follows:
 */
prog_uint8_t can_filter[] =
{
        // Group 0
        MCP2515_FILTER(0),                              // Filter 0
        MCP2515_FILTER(0),                              // Filter 1

        // Group 1
        MCP2515_FILTER_EXTENDED(0),             // Filter 2
        MCP2515_FILTER_EXTENDED(0),             // Filter 3
        MCP2515_FILTER_EXTENDED(0),             // Filter 4
        MCP2515_FILTER_EXTENDED(0),             // Filter 5

        MCP2515_FILTER(0),                              // Mask 0 (for group 0)
        MCP2515_FILTER_EXTENDED(0),             // Mask 1 (for group 1)
};

void errorresponse(uint8_t code, can_t *msg) {
	uartPutc(0xff); //preamble
	uartPutc(0xff);
	uartPutc(0x00);
	uartPutc(0x00);
	
	uartPutc(0xff); //id
	uartPutc(0xff);
	uartPutc(0xff);
	uartPutc(code);
	
	uartPutc(0x01); //ext,rtr
	uartPutc(0x04); //length
	
	uartPutc(msg->id>>24);
	uartPutc(msg->id>>16);
	uartPutc(msg->id>>8);
	uartPutc(msg->id);
	uartPutc(0x00);
	uartPutc(0x00);
	uartPutc(0x00);
	uartPutc(0x00);
	
	uartPutc(0x00); //extra bytes(not needed, but should reset the receiver)
	uartPutc(0xff);
	uartPutc(0x00);
}

int main(void) {
	// Format:	0xff,0xff,0x00,0x00 (Preamble)
	//			4 bytes (ID, right aligned)
	//			0x01 (extended id) or 0x00 (short id)
	//			1 byte (length of message)
	//			8 bytes (message, stuff with 0x00)
	//			as much 0x00s as you like
	uint8_t rx_pos=0;
	
	LED1_INIT();
	LED2_INIT();
	
	LED1_ON();
	LED2_ON();
	
	uartInit(3);		//230.4k @ 14,74MHz
// 	systemUartInit(51);			//9.6k @ 8MHz
	
// 	uartPuts("hello\n");
	
	can_init(BITRATE_125_KBPS);
	can_static_filter(can_filter);
// 	can_set_mode(LOOPBACK_MODE);
	
	LED2_OFF();
	
	sei();
	
	can_t from_can, to_can;
	
	for(;;) {
		if(rx_pos==0) LED1_ON();
		else LED1_OFF();
		
		if (uartAvailable()) {
			if(rx_pos==0 || rx_pos==1) {			//preamble pt. 1
				if (uartGetc()==0xff) rx_pos++;
				else rx_pos=0;
			}
			else if(rx_pos==2 || rx_pos==3) {			//preamble pt. 2
				if (uartGetc()==0x00) rx_pos++;
				else rx_pos=0;
			}
			
			else if(rx_pos==4 && uartAvailable()>=4) {	//ID
				uint8_t i;
				to_can.id=0;
				for(i=0; i<4; i++) {
					to_can.id<<=8;
					to_can.id += uartGetc();
				}
				
				rx_pos=8;
			}
			
			else if(rx_pos==8) {						//Extended and rtr
				uint8_t extfield=uartGetc();
				
				if (extfield&0x01)	to_can.flags.extended=1;
				else				to_can.flags.extended=0;
				if (extfield&0x02)	to_can.flags.rtr=1;
				else				to_can.flags.rtr=0;
				
				rx_pos++;
					
			}
			
			else if(rx_pos==9) {						//Length
				to_can.length=uartGetc();
				if(to_can.length<=8) rx_pos++;
				else rx_pos=0;
			}
			
			else if(rx_pos==10 && uartAvailable()>=8) {
				uint8_t i;
				for(i=0; i<8; i++) {
					to_can.data[i]=uartGetc();
				}
				
				if((to_can.id>>24)<0x0e) {				// valid id?
					if(can_check_free_buffer()) {
						if (!can_send_message(&to_can)) {
							errorresponse(SEND_ERROR,&to_can);
						}
					} else {
						errorresponse(BUFFER_FULL,&to_can);
					}
				} else {
					errorresponse(MALFORMED_MSG, &to_can);
				}
				rx_pos=0;
			}
		}
			
		// Try to read the message
		if (can_get_message(&from_can)) {
			LED2_TOGGLE();
			
			uartPutc(0xff);
			uartPutc(0xff);
			uartPutc(0x00);
			uartPutc(0x00);
			
			uartPutc( (uint8_t)(from_can.id>>24) );
			uartPutc( (uint8_t)(from_can.id>>16) );
			uartPutc( (uint8_t)(from_can.id>>8) );
			uartPutc( (uint8_t)from_can.id );
			
			uint8_t flags=0;
			if(from_can.flags.extended)	flags |= 0x01;
			if(from_can.flags.rtr)		flags |= 0x02;
			uartPutc(flags);
			
			uartPutc( from_can.length );
			
			uint8_t i;
			for(i=0; i<8; i++)
				uartPutc(from_can.data[i]);
			
			uartPutc(0x00);
			uartPutc(0xff);
			uartPutc(0x00);
		}
		
	}
	
	return 0;
}
