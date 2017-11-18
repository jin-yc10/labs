
#include "IRremote.h"
#include "IRremoteInt.h"

// = 40e6/38e3;

void set_pwm_freq(unsigned int hz) {
    CloseTimer3();
    generate_period = 40e6/hz;
//    printf("hz=%d, generate_period=%d\n", hz, generate_period );
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1, generate_period);
}

//+=============================================================================
void  sendRaw (const unsigned int buf[],  unsigned int len,  unsigned int hz) {
	// Set IR carrier frequency
	enableIROut(hz);
    unsigned int i;
	for (i = 0;  i < len;  i++) {
		if (i & 1)  space(buf[i]) ;
		else        mark (buf[i]) ;
	}
	space(0);  // Always end with the LED off
}

//+=============================================================================
// Sends an IR mark for the specified number of microseconds.
// The mark output is modulated at the PWM frequency.
//
void  mark (unsigned int time) {
    // TODO: enable pwm
	TIMER_ENABLE_PWM; // Enable pin 3 PWM output
	if (time > 0) custom_delay_usec(time);
}

//+=============================================================================
// Leave pin off for time (given in microseconds)
// Sends an IR space for the specified number of microseconds.
// A space is no output, so the PWM output is disabled.
//
void  space (unsigned int time) {
	TIMER_DISABLE_PWM; // Disable pin 3 PWM output
	if (time > 0) custom_delay_usec(time);
}

//+=============================================================================
// Enables IR output.  The khz value controls the modulation frequency in kilohertz.
// The IR output will be on pin 3 (OC2B).
// This routine is designed for 36-40KHz; if you use it for other values, it's up to you
// to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
// TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
// controlling the duty cycle.
// There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
// To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
// A few hours staring at the ATmega documentation and this will all make sense.
// See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.
//
void  enableIROut (int khz) {
//    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1, generate_period);
    set_pwm_freq(khz*1000);
    OpenOC3(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE , 0, 0);
    PPSOutput(4, RPB0, OC3);
//    TIMER_DISABLE_PWM
}

//+=============================================================================
// Custom delay function that circumvents Arduino's delayMicroseconds limit
unsigned long micros() {
    return counter_50us*50;
}

void custom_delay_usec(unsigned long uSecs) {
    // TODO: add delay
  if (uSecs > 4) {
    unsigned long start = micros();
    unsigned long endMicros = start + uSecs - 4;
    if (endMicros < start) { // Check if overflow
      while ( micros() > start ) {} // wait until overflow
    }
    while ( micros() < endMicros ) {} // normal wait
  }
}

