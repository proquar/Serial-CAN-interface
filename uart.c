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

#include "uart.h"


#define ENABLE_RX_INT()		UCSRB |= (1<<RXCIE)
#define DISABLE_RX_INT()	UCSRB &= ~(1<<RXCIE)
#define ENABLE_TX_INT()		UCSRB |= (1<<UDRIE)
#define DISABLE_TX_INT()	UCSRB &= ~(1<<UDRIE)

#define TXBUFFERSIZE_BITMASK	((1<<TXBUFFERSIZE)-1)
#define RXBUFFERSIZE_BITMASK	((1<<RXBUFFERSIZE)-1)

volatile struct {
	char buffer[1<<RXBUFFERSIZE];
	uint8_t readpos;
	uint8_t writepos;
} rxbuffer;

volatile struct {
	uint8_t buffer[1<<TXBUFFERSIZE];
	uint8_t readpos;
	uint8_t writepos;
} txbuffer;



void uartInit(uint8_t baudrate) {
	//enable transmitter+receiver, 8 databits
	UCSRB = (1<<TXEN)|(1<<RXEN);
	
	//asynchronous, no parity, 1 stopbit, 8 databits
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);

	UBRRH = 0;
	UBRRL = baudrate;
	
	rxbuffer.readpos=0;
	rxbuffer.writepos=0;
	
	txbuffer.readpos=0;
	txbuffer.writepos=0;
	
	ENABLE_RX_INT();
}



void uartPutc(uint8_t byte) {
	uint8_t tmpwritepos=txbuffer.writepos;
	txbuffer.buffer[tmpwritepos++]=byte;
	
	if( (tmpwritepos&TXBUFFERSIZE_BITMASK) == txbuffer.readpos ) {
		tmpwritepos--;
	}
	txbuffer.writepos=tmpwritepos&TXBUFFERSIZE_BITMASK;
	ENABLE_TX_INT();
}

void uartPuts(char* str) {
	while(*str!='\0') {
		uartPutc(*str++);
	}
}

ISR( USART_UDRE_vect ) {
	if ( txbuffer.readpos != txbuffer.writepos ) {
		UDR=txbuffer.buffer[txbuffer.readpos++];
		
		txbuffer.readpos &= TXBUFFERSIZE_BITMASK;
	} else {
		DISABLE_TX_INT();
	}
}

uint8_t uartAvailable(void) {
	return (rxbuffer.writepos-rxbuffer.readpos)&RXBUFFERSIZE_BITMASK;
}

uint8_t uartGetc(void) {
	if ( rxbuffer.readpos != rxbuffer.writepos ) {
		uint8_t toreturn = rxbuffer.buffer[rxbuffer.readpos];
		
		rxbuffer.readpos++;
		rxbuffer.readpos &= RXBUFFERSIZE_BITMASK;
		
		return toreturn;
	} else {
		return 0;
	}
}

ISR( USART_RXC_vect ) {
	rxbuffer.buffer[rxbuffer.writepos++]=UDR;
	
	if( (rxbuffer.writepos&RXBUFFERSIZE_BITMASK) == rxbuffer.readpos ) {
		rxbuffer.writepos--;
	}
	rxbuffer.writepos &= RXBUFFERSIZE_BITMASK;
}
