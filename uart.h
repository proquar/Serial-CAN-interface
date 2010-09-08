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

#ifndef UART_H
#define UART_H

#define RXBUFFERSIZE  6
#define TXBUFFERSIZE  6

void uartInit(uint8_t);

/* use this function to put a single bytes into the transmitbuffer */
void uartPutc(uint8_t);
void uartPuts(char*);

#define uartPutcSync(CHAR) while ( !(UCSRA & (1<<UDRE)) ); UDR=CHAR


uint8_t uartAvailable(void);
uint8_t uartGetc(void);

#endif
 
