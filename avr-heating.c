/**
    avr-heating.c
    Copyright (C) 2012 Jacob Bower

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define F_CPU 7372800UL
#define UART_BAUD 9600

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include <stdint.h>

#define MAX_EVENTS 32

typedef struct {
	unsigned int on_off : 1;
	unsigned int day    : 3; // 0 => invalid
	unsigned int minute : 11;
} event_t;

static event_t EEMEM event_table[MAX_EVENTS];

#define MINUTE_EVENT 0x01
#define TABLE_WRITE_EVENT 0x02
static volatile uint8_t recent_events = 0;
static volatile uint8_t timed_operation_enabled = 0;
static event_t new_entry;
static uint8_t new_entry_pos;

static volatile uint8_t state = 0;
static volatile uint8_t day = 1;
static volatile uint16_t minute = 0;
static volatile uint16_t overflows = 0;

static volatile uint8_t prev_button_pressed = 0;

ISR(TIMER0_OVF_vect)
{
	if(timed_operation_enabled) {
		// If button pressed...
		if(!(PIND & _BV(6))) {
			if(!prev_button_pressed) {
				state = ~state;
				PORTB = state;
			}
			prev_button_pressed = 1;
		} else
			prev_button_pressed = 0;
	}

	if(++overflows > (uint16_t)((F_CPU * 60UL) / 1024UL / 256UL)) {
		if(++minute == (uint16_t)(60UL * 24UL)) {
			minute = 0;
			if(++day == 8)
				day = 1;
		}
			
		recent_events |= MINUTE_EVENT;
		overflows = 0;
	}
}


void checkTable(void)
{
	event_t entry;

	for(uint8_t i = 0; i < MAX_EVENTS; i++) {
		eeprom_read_block(
			(void*)&entry,
			(const void*)&event_table[i],
			sizeof(event_t));

		if(entry.day == day && entry.minute == minute) {
			state = -((uint8_t)entry.on_off);
			PORTB = state;
			return;
		}
	}
}


/*
 * Initialize the UART to 9600 Bd, tx/rx, 8N1.
 */
void uart_init(void)
{
#if F_CPU < 2000000UL && defined(U2X)
	UCSRA = _BV(U2X);             /* improve baud rate error by using 2x clk */
	UBRRL = (F_CPU / (8UL * UART_BAUD)) - 1;
#else
	UBRRL = (F_CPU / (16UL * UART_BAUD)) - 1;
#endif
//	UCSRC = (1 << UCSZ0) | (1 << UCSZ1);
  	UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE); /* tx/rx/interrupt enable */
}

uint8_t uart_getchar(void)
{
	loop_until_bit_is_set(UCSRA, RXC);

	return UDR;
}

void uart_putchar(char c)
{
	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = c;
}

ISR(USART_RX_vect)
{
	typedef enum {
		COMMAND,
		SET_DAY,
		SET_MINUTE_BYTE2,
		SET_MINUTE_BYTE1,
		SET_ENTRY_POS,
		SET_ENTRY_ONOFF,
		SET_ENTRY_MINUTE_BYTE2,
		SET_ENTRY_MINUTE_BYTE1,
		SET_ENTRY_DAY,
		GET_ENTRY
	} uart_rx_state_t;

	static volatile uart_rx_state_t uart_rx_state = COMMAND;
	static volatile uint8_t minute_tmp;

	switch(uart_rx_state)
	{
		case COMMAND:
			switch(UDR)
			{
				case 0x01:
					state = 0;
					PORTB = 0x00;
					break;

				case 0x02:
					state = 0xFF;
					PORTB = 0xFF;
					break;

				case 0x03:
					uart_putchar(state);
					break;

				case 0x04:
					uart_rx_state = SET_DAY;
					break;
				
				case 0x05:
					uart_rx_state = SET_MINUTE_BYTE2;
					break;

				case 0x06:
					uart_rx_state = SET_ENTRY_POS;
					break;

				case 0x07:
					timed_operation_enabled = 0;
					break;

				case 0x08:
					timed_operation_enabled = 1;
					break;

				case 0x09:
					uart_putchar(day);
					uart_putchar(minute >> 8);
					uart_putchar(minute);
					break;

				case 0x0A:
					uart_rx_state = GET_ENTRY;
					break;

				case 0x0B:
					uart_putchar(timed_operation_enabled);
					break;

				case 0x0C:
					uart_putchar(PIND);
					break;
			}
			break;

		case SET_DAY:
			day = UDR;
			overflows = 0;
			uart_rx_state = COMMAND;
			break;

		case SET_MINUTE_BYTE2:
			minute_tmp = UDR;
			uart_rx_state = SET_MINUTE_BYTE1;
			break;

		case SET_MINUTE_BYTE1:
			minute = (((uint16_t)minute_tmp) << 8) | UDR;
			overflows = 0;
			uart_rx_state = COMMAND;
			break;

		case SET_ENTRY_POS:
			new_entry_pos = UDR;
			uart_rx_state = SET_ENTRY_ONOFF;
			break;

		case SET_ENTRY_ONOFF:
			new_entry.on_off = UDR;
			uart_rx_state = SET_ENTRY_MINUTE_BYTE2;
			break;

		case SET_ENTRY_MINUTE_BYTE2:
			new_entry.minute = ((uint16_t)UDR) << 8;
			uart_rx_state = SET_ENTRY_MINUTE_BYTE1;
			break;

		case SET_ENTRY_MINUTE_BYTE1:
			new_entry.minute = new_entry.minute | UDR;
			uart_rx_state = SET_ENTRY_DAY;
			break;

		case SET_ENTRY_DAY:
			new_entry.day = UDR;
			uart_rx_state = COMMAND;
			recent_events |= TABLE_WRITE_EVENT;
			break;

		case GET_ENTRY:
			eeprom_read_block(
				(void*)&new_entry,
				(const void*)&event_table[UDR],
				sizeof(event_t));
			uart_putchar(new_entry.on_off);
			uart_putchar(new_entry.day);
			uart_putchar(new_entry.minute >> 8);
			uart_putchar(new_entry.minute);
			uart_rx_state = COMMAND;
			break;
	}
}

static void timer_init(void)
{
	// Enable clk / 1024 per tick on TIMER0
	TCCR0B |= _BV(CS02) | _BV(CS00);

	// Enable interrupts for TIMER0
	TIMSK |= _BV(TOIE0);
}

int main(void)
{
	// Port B set to all outputs and initially set off.
	DDRB = 0xFF;
	PORTB = 0x00;

	// Port D set to all inputs with pull-up resistors enabled.
	DDRD = 0x00;
	PORTD = 0xFF;

	uart_init();
	timer_init();

	for(;;) {
		sleep_enable();
		sei();
		sleep_cpu();
		cli();
		sleep_disable();

		if(timed_operation_enabled && recent_events & MINUTE_EVENT)
			checkTable();
		if(recent_events & TABLE_WRITE_EVENT) 
			eeprom_write_block(
				(const void*)&new_entry,
				(void*)&event_table[new_entry_pos],
				sizeof(event_t));
		recent_events = 0;
	}
}
