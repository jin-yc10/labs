
/**
 * This is a very small example that shows how to use
 * === OUTPUT COMPARE  ===
/*
 * File:        PWM example
 * Author:      Bruce Land
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
// threading library
#include "pt_cornell_1_2_1.h"

static struct pt pt_time, pt_cmd, pt_adc;

// system 1 second interval tick
int sys_time_seconds ;

//The actual period of the wave
int generate_period = 40000 ;
int pwm_on_time = 0 ;
int pwm_increment = 20;

#define USE_FIX_POINT

#ifdef USE_FIX_POINT
typedef signed int fix16 ;
#define multfix16(a,b)  ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) 
#define float2fix16(a)  ((fix16)((a)*65536.0))
#define fix2float16(a)  ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b)   ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a)    (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a)     abs(a)
#endif


static float kp, ki, kd;
static float target;
static float current;
static float error, last_error, derivative, sigma_error;
static float control_val;

// == Timer 2 ISR =====================================================
// just toggles a pin for timeing strobe
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    // generate a trigger strobe for timing other events
    mPORTBSetBits(BIT_0);
    // clear the timer interrupt flag
    mT2ClearIntFlag();
    mPORTBClearBits(BIT_0);
    
    error = target - current;
    sigma_error += error;
    derivative = error - last_error;
    control_val = kp*error+ki*sigma_error+kd*derivative;
      last_error = error;
//    error = (target - last_error);
//    
//    proportional_cntl = kp * error;
//    differential_cntl = kd * (error - last_error); 
//    integral_cntl = integral_cntl + error ;
//    if (error == last_error){
//        integral_cntl = 0;
//    }
//    control_val =  proportional_cntl + differential_cntl + ki * integral_cntl ;
//    //uncomment this next statement to get a passive pendulum
//    //control_val = 0;  
//    
//   // physical bound of the PWM 
//   if (control_val<0) {
//        control_val = 0;
//    }
//   // physical bound of the PWM 
//   if (control_val>39999) {
//        control_val = 39999 ;
//    }
}


// === Command Thread ======================================================
// The serial interface
static char cmd[16]; 
static int value;

static PT_THREAD (protothread_adc(struct pt *pt)) {
    PT_BEGIN(pt);
    static int adc_9;
    static float V, position;
    static fix16 Vfix, ADC_scale ;
    
    ADC_scale = float2fix16(3.3/1023.0); //Vref/(full scale)
            
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(60);
        adc_9 = ReadADC10(0);   // read the result of channel 9 conversion from the idle buffer
        AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below
        V = (float)adc_9 * 3.3 / 1023.0 ; // Vref*adc/1023
        // convert to fixed voltage
    }
    PT_END(pt);
} // animation thread

static PT_THREAD (protothread_cmd(struct pt *pt)) {
    PT_BEGIN(pt);
    static int step;
    step = generate_period / 100;
    while(1) {
        PT_YIELD_TIME_msec(100);
        if(pwm_on_time >= generate_period ) pwm_increment = -step;
        if(pwm_on_time <= 0) pwm_increment = step;
        pwm_on_time += pwm_increment;
        SetDCOC3PWM(pwm_on_time);
    } // END WHILE(1)
    PT_END(pt);
} // thread 3

// === One second Thread ======================================================
// update a 1 second tick counter
static PT_THREAD (protothread_time(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // thread 4

// === Main  ======================================================

int main(void)
{
    // set up timer2 to generate the wave period
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, generate_period);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

    // set up compare3 for PWM mode
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time, pwm_on_time); //
    // OC3 is PPS group 4, map to RPB9 (pin 18)
    PPSOutput(4, RPB9, OC3);

    // === config the uart, DMA, vref, timer5 ISR ===========
    PT_setup();

    // === setup system wide interrupts  ====================
    INTEnableSystemMultiVectoredInt();

    PT_INIT(&pt_cmd);
    PT_INIT(&pt_time);

    // schedule the threads
    while(1) {
        PT_SCHEDULE(protothread_cmd(&pt_cmd));
        PT_SCHEDULE(protothread_time(&pt_time));
    }
} // main
