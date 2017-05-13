#include "serial.h"

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define USE_U2X 0
#define UBRR_CALC(baud,f_osc,u2x) (((uint32_t)(f_osc)/(8*(2-(u2x))*(uint32_t)(baud)))-1)

/* static FILE structure for stdout */
static int uart_putchar(char c, FILE *stream);
static int uart_getchar(FILE *stream);

static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
static FILE uart_stdin  = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

#define UART_BUFSIZE    32

static char uart_outbuf[UART_BUFSIZE];
static volatile uint8_t uart_writep;
static volatile uint8_t uart_readp;

ISR(USART_UDRE_vect){
	uint8_t rp = uart_readp;
	UDR0 = uart_outbuf[rp];

	rp = (rp + 1)%UART_BUFSIZE;
	uart_readp = rp;

	if(rp == uart_writep){ /* empty */
		UCSR0B &= ~ _BV(UDRIE0); /* turn off Data Register Empty Interrupt */
	}
}

static inline void
uart_put_into_outbuf(unsigned char c){
	uint8_t wp;

	while(1){
		cli();
		wp = uart_writep;
		uart_outbuf[wp]=c;
		wp = (wp + 1)%UART_BUFSIZE;
		if(wp == uart_readp){ /* full! */
			sei();
			continue;
		}
		uart_writep = wp;
		if(!(UCSR0B & _BV(UDRIE0))) /* if Data Register Empty Interrupt... */
			UCSR0B |= _BV(UDRIE0); /* is off, turn it on. */
		sei();
		break;
	}
}

static int
uart_putchar(char c, FILE *stream)
{
	(void) stream; /* silence warning */

	if(c == '\r')
		return 0;
	if(c == '\n')
		uart_put_into_outbuf('\r');
	uart_put_into_outbuf(c);
	return 0;
}

static int
uart_getchar(FILE *stream)
{
	(void) stream; /* silence warning */

	/* wait for Receive Complete bit to be set */
	while(!serial_status()); /* wait... */
	return UDR0;
}

/* there is a byte to be read */
int
serial_status(){
	return UCSR0A & _BV(RXC0);
}

void
serial_init(){
	uint16_t ubrr = UBRR_CALC(9600,F_CPU,USE_U2X);

	stdout = &uart_stdout;
	stdin  = &uart_stdin;

	/* when writing UCSR0C as UBRRH, the topmost 4 bits
	   must be written as zero! */
	UBRR0H = (ubrr >> 8) & 0x0F;
	UBRR0L =  ubrr & 0xff;
	
	UCSR0C = _BV(UCSZ01)|_BV(UCSZ00); /* Async, 8N1 */
	UCSR0A = (USE_U2X ? _BV(U2X0) : 0);
	UCSR0B = _BV(RXEN0)|_BV(TXEN0);
};
