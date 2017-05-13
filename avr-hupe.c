#include "serial.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#define NSTEPS 32

#if NSTEPS == 16
static const uint8_t table[NSTEPS] = {
	  0,   49,   97,  142,
	181,  212,  236,  251,
	255,  251,  236,  212,
	181,  142,   97,   49
};
#else
#if NSTEPS == 32
static const uint8_t table[NSTEPS] = {
	0, 24, 49, 74, 97, 120, 141, 161,
	180, 197, 212, 224, 235, 244, 250, 253,
	255, 253, 250, 244, 235, 224, 212, 197,
	180, 161, 141, 120, 97, 74, 49, 24
};
#endif
#endif

#define PHASEMASK (((2*NSTEPS)<<8) - 1)

static volatile uint16_t phaseinc = 112;   /* default ~ 50 Hz */
static volatile uint16_t phase = 0;        /* MSB: step, LSB: fractional */
static volatile uint8_t gain = 0;          /* PWM gain */

ISR(TIMER2_OVF_vect) {
	uint16_t p;       /* phase */
	uint8_t s;        /* array step */
	uint8_t va, vb;   /* value for PWM a and b */
	uint8_t g = gain; /* scale factor for PWM values */

	p = phase & PHASEMASK;
	p += phaseinc;
	phase = p & PHASEMASK;

	s = (p >> 8) & 0xff; /* steps are the 8 MSB of phase */

	if (s & NSTEPS)
		PORTD |= _BV(7);
	else
		PORTD &= ~_BV(7);

	va = vb = 0;
	if (s >= NSTEPS) {
		vb = table[s-NSTEPS];
		vb = (vb * g) >> 8;
	} else {
		va = table[s];
		va = (va * g) >> 8;
	}

	if (va == 0 && (TCCR0A & _BV(COM0A1))) {
		TCCR0A &= ~_BV(COM0A1); /* turn off PWM A */
	}
	if (va) {
		if (!(TCCR0A & _BV(COM0A1)))
			TCCR0A |=  _BV(COM0A1); /* turn on PWM A */
		OCR0A = va;
	}

	if (vb == 0 && (TCCR0A & _BV(COM0B1))) {
		TCCR0A &= ~_BV(COM0B1); /* turn off PWM B */
	}
	if (vb) {
		if (!(TCCR0A & _BV(COM0B1)))
			TCCR0A |=  _BV(COM0B1); /* turn on PWM B */
		OCR0B = vb;
	}
}


/*
   PD6 = OC0A = Arduino Pin #6
   PD5 = OC0B = Arduino PIN #5
*/

int
main()
{
	char c;

	serial_init();

	DDRD |= _BV(4);
	DDRD |= _BV(5);
	DDRD |= _BV(6);
	DDRD |= _BV(7);

	PORTD &= ~_BV(4); /* enable */
	PORTD &= ~_BV(5); /* Rpwm */
	PORTD &= ~_BV(6); /* Lpwm */
	PORTD &= ~_BV(7);

	/* enable fast PWM mode #3, TOP=0xff */
	TCCR0A = _BV(WGM01)|_BV(WGM00);
	TCCR0B = _BV(CS01);  /* 16MHz / 256 / 8 = 8 kHz */
	OCR0A = 0x0;
	OCR0B = 0x0;

	TCCR2B = _BV(CS21);  /* 16 MHz / 256(top) / 8 prescale = 7.8 kHz */
	TIMSK2 = _BV(TOIE2); /* timer overflow interrupt enable */

	sei();
	puts_P(PSTR("\n\n***\n*** AVR Hupe\n***\n\n"
		"?: status, +/-: voltage gain </>: frequency\n"
		"0..9: gain 0, 25...225 (steps of 25)\n\n"));

	while (1) {
		if (!serial_status())
			continue;
		c = getchar();
		if (c == '?') {
			printf_P(PSTR("current phase is 0x%04x\n"), phase);
			printf_P(PSTR("phaseinc = %u\n"), phaseinc);
			printf_P(PSTR("gain = %u\n"), gain);
			if (PORTD & _BV(4))
				puts_P(PSTR("output enabled"));
			else
				puts_P(PSTR("output disabled"));


		}
		if (c == '+') {
			if (gain < 0xfc) {
				gain++;
			} else {
				gain = 0xff;
				puts_P(PSTR("gain max"));
			}
		}
		if (c == '-') {
			if (gain >= 0x04) {
				gain-=4;
			} else {
				gain = 0x00;
				puts_P(PSTR("gain min"));
			}
		}
		if (c == '<') {
			if (phaseinc > 0x0010) {
				phaseinc -= 0x10;
			} else {
				puts_P(PSTR("freq min"));
			}
		}
		if (c == '>') {
			if (phaseinc < 0x0400) {
				phaseinc += 0x10;
			} else {
				puts_P(PSTR("freq max"));
			}
		}

		if (c == 'e') {
			PORTD |= _BV(4);
			puts_P(PSTR("output enabled"));
		}
		if (c == 'd') {
			PORTD &= ~_BV(4);
			puts_P(PSTR("output disabled"));
		}
		if (c >= '0' && c <= '9') {
			uint8_t v = (c-'0')*25;
			printf_P(PSTR("%c: gain %u\n"), c, v);
			gain = v;
		}
	}


	while(1);

}