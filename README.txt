avr-hupe : A very quick one-off test to generate an approximation of a
   sinewave on a H-bridge, to test a scooter horn.


Stupid little program to generate a biphase PWM signal on a H-bridge.

   PD7        = Arduino Pin #7  Debug signal, in phase with output
   PD6 = OC0A = Arduino Pin #6  H/L: output of right pin of H-bridge
   PD5 = OC0B = Arduino PIN #5  H/L: output of left  pin H-bridge
   PD4        = Arduino PIN #4  H:   enable both sides of H-Bridge


Assuming a 16 MHz XTAL / clock frequency.

PWM is approximately 8 kHz and uses Timer/Counter 0. This had to be
chosen that slow because the slew-rate + switch on delay on a BTS7960
is typ. 10µs and shorter pulses with faster PWM rate wouldn't make the
bridge switch.

The sine table is 32 steps long (NSTEPS) and the PWM values are currently
updated at the same rate using the overflow interrupt of Timer/Counter2
(two timers, so that I could possibly run the PWM faster).

A "phase increment" is added to the "phase" counter on every tick of T/C2
and its MSB are used as index in the sine table (which only contains the
positive half-period, so it takes 2*NSTEPS to complete one cycle).

Phase increment minimum is currently set to 16, so it takes
 2*NSTEPS * 256 / phaseinc = 1024 steps, at 8kHz -> aprox. 8 Hz min freq
Phase increment maximum is currently set to 1024, so...
 2*NSTEPS * 256 / phaseinc = 16 steps, at 8kHz -> approx 500 Hz max freq

The complete system will generate an aproximation of a sinewave between
the two outputs of the H-bridge. Serial port on 9600 baud allows control:

  +/-, 0..9: adjust gain
  </>: adjust frequency
  e/d: toggle PD7, used as an enable pin on the bridge.
  ?  : give status


	$ cu -l ttyUSB0 -s 9600
	Connected.

	***
	*** AVR Hupe
	***

	?: status, +/-: voltage gain </>: frequency
	0..9: gain 0, 25...225 (steps of 25)


	current phase is 0x0ff0
	phaseinc = 112
	gain = 0
	output disabled

