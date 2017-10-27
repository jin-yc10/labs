
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
#include "tft_master.h"
#include "tft_gfx.h"
// threading library
#include "pt_cornell_1_2_1.h"

static struct pt pt_time, pt_display, pt_adc;
static char buffer[60];

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

#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 15 MHz DAC clock

static int kp = 300, ki_inv = 16, kd = 16000;
static int target;
static int current;
static int error = 0, last_error, derivative, sigma_error = 0;
static int control_val;

static int last_errors[5];
static int last_error_idx = 0;

static unsigned char state = 1; // 0 Begin, 1 RUNNING
static int isr_cnt = 0;
// == Timer 2 ISR =====================================================
// just toggles a pin for timeing strobe

char sign(int v) {
    if( v > 0 ) return 1;
    else if( v == 0 ) return 0;
    else return -1;
}

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    static char i;
    static int last_error_sum;
    isr_cnt++;
    // clear the timer interrupt flag
    mT2ClearIntFlag();
    
    error = target - current;
    sigma_error += error;
    
    if(last_error_idx < 4) {
        derivative = 0;
    } else {
        last_error = last_errors[0];
        derivative = (error - last_error)/5;
    }
//      if (abs(error) > 30){
//        sigma_error = 0;
//    }
    control_val = kp * error + kd * derivative + sigma_error / ki_inv;

    if( control_val < 0 ) control_val = 0;
    if( control_val > 20000 ) control_val = 20000;
    
    if( state == 1 ) {
        SetDCOC3PWM(control_val);    
    } else {
        SetDCOC3PWM(0);    
    }
    if(last_error_idx < 4) {
        last_errors[last_error_idx] = error;
        last_error_idx++;
    } else {
        for(i=1; i<5; i++) {
            last_errors[i-1] = last_errors[i];
        }
        last_errors[4] = error;
    }    
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
static int adc_11, adc_1, raw_11;

static PT_THREAD (protothread_adc(struct pt *pt)) {
    PT_BEGIN(pt);        
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(30);
        adc_11 = ReadADC10(0);
        adc_1 = ReadADC10(1); 
        raw_11 = ( adc_11 - 512 ) / 2;
        current = - ( adc_1 - 522 );
        // end SPI transaction from last interrupt cycle
        // CS low to start transaction
        mPORTBSetBits(BIT_4);
        mPORTBClearBits(BIT_4); // start transaction
        WriteSPI2( DAC_config_chan_A | (short)((float)control_val/32000.0*4000.0));
        PT_YIELD_TIME_msec(5);
        mPORTBSetBits(BIT_4);
        mPORTBClearBits(BIT_4); // start transaction
        WriteSPI2( DAC_config_chan_B | (short)((float)(current+501)/1023.0*4000.0));
//        SetDCOC3PWM((int)((float)adc_11/750.0*20000.0));
//        AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below
//        V = (float)adc_11 * 3.3 / 1023.0 ; // Vref*adc/1023
//        target = V;
//        current = (float)adc_1 * 3.3 / 1023.0 ;
    }
    PT_END(pt);
} // animation thread

static char first_key = 0;

static PT_THREAD (protothread_display(struct pt *pt)) {
    PT_BEGIN(pt);
    
    static unsigned int temp = 0; 
    static unsigned int para_idx = 0;
    static unsigned char key_code = 0;
    
    while(1) {
        key_code = 0;
        if( mPORTAReadBits(BIT_2)==0 && temp==0 ) {
            temp |= 1;
            key_code = 1;
        }
        if( mPORTAReadBits(BIT_2)==0x04 ) {
            temp &= ~(1);
        }
        if( mPORTAReadBits(BIT_3)==0 && temp==0 ) {
            temp |= 2;
            key_code = 2;
        }
        if( mPORTAReadBits(BIT_3)==0x08 ) {
            temp &= ~(2);
        }
        
        if( key_code == 1 ) { 
            first_key = 1;
            if( state == 0 ) {// BEGIN
                state = 1;
                tft_fillRoundRect(0 ,0, 320, 240, 1, ILI9340_BLACK);  
            } else if( state == 1) { // RUNNING
                state = 0;
                tft_fillRoundRect(0 ,0, 320, 240, 1, ILI9340_BLACK);  
            }
        } else if( key_code == 2 ) {
            first_key = 1;
            if( state == 0 ) {
                para_idx ++;
                if( para_idx > 2 ) para_idx = 0;
            } else {
                // do nothing while running
            }
        }
        
        if( state == 1 ) {
            if( abs(raw_11 - target) > 5 && first_key == 1) {
                target = raw_11;
            }
            PT_YIELD_TIME_msec(60);
            tft_setCursor(40, 40);
            tft_fillRoundRect(40, 40, 280, 80, 1, ILI9340_BLACK);
            tft_setTextColor(ILI9340_YELLOW);  tft_setTextSize(2); 
            sprintf(buffer, "%d,%d,%d,%d", control_val, target/3, current/3, error);
            tft_writeString(buffer);
            tft_setCursor(40, 80);
            sprintf(buffer, "%d %d %d %d", error, last_error, derivative, sigma_error);
            tft_writeString(buffer);
            tft_setTextSize(1); 
            tft_fillRoundRect(40, 120, 200, 20, 1, ILI9340_BLACK);
            tft_setCursor(40, 120);
            sprintf(buffer, "p=%d d=%d i=%d", kp * error, kd * derivative, sigma_error / ki_inv);
            tft_writeString(buffer);
        } else if( state == 0 ) {
            PT_YIELD_TIME_msec(60);            
            tft_setTextSize(2); 
            tft_fillRoundRect(40, 20, 40, 40, 1, ILI9340_BLACK);
            tft_setCursor(40, 20);
            switch( para_idx ) {
                case 0: // kp
                    kp = 1.0f*(raw_11+255);
                    sprintf(buffer, "kp");
                    break;
                case 1: // ki
//                    ki = 0.001f*(raw_11+255);
                    sprintf(buffer, "ki");
                    break;
                case 2: // kd
                    kd = 4.0f*(raw_11+255);
                    sprintf(buffer, "kd");
                    break;
            }
            
            tft_writeString(buffer);
            tft_fillRoundRect(40, 40, 280, 40, 1, ILI9340_BLACK);
            tft_setCursor(40, 40);
//            sprintf(buffer, "kp=%.1f ki=%.1f kd=%.1f", kp, ki, kd);
            tft_writeString(buffer);
        }           
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
        PT_YIELD_TIME_msec(1000);
        sys_time_seconds++ ;
        
        tft_fillRoundRect(40, 160, 80, 40, 1, ILI9340_BLACK);
        tft_setTextSize(2); 
        tft_setCursor(40, 160);
        sprintf(buffer, "isr=%d", isr_cnt);
        isr_cnt = 0;
        tft_writeString(buffer);
        if( sys_time_seconds < 5 ) {
            target = 0;
        } else if( sys_time_seconds < 10 ) {
            target = 75;
        } else if( sys_time_seconds < 15 ) {
            target = -90;
        } else if( first_key == 0 ) {
            target = 0;
        }
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // thread 4

// === Main  ======================================================

int main(void) {
    // set up timer2 to generate the wave period
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, generate_period);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

    // set up compare3 for PWM mode
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , pwm_on_time, pwm_on_time); //
    // OC3 is PPS group 4, map to RPB9 (pin 18)
    PPSOutput(4, RPB9, OC3);
    
    CloseADC10();	// ensure the ADC is off before setting the configuration
    #define PARAM1 ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON //
	#define PARAM2 ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_ON
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_15 | ADC_CONV_CLK_Tcy //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2
	#define PARAM4 ENABLE_AN11_ANA | ENABLE_AN1_ANA // pin 24(RB13), pin3(RA1)
	#define PARAM5 SKIP_SCAN_ALL
    // configure to sample AN11, AN10 
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 | ADC_CH0_NEG_SAMPLEB_NVREF | ADC_CH0_POS_SAMPLEB_AN1 ); 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above
	EnableADC10(); // Enable the ADC
    
    PPSOutput(2, RPB5, SDO2);
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
    
    mPORTASetPinsDigitalIn(BIT_2|BIT_3);
    
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(1); // FOR HORIZONTAL 320x240
    
    // === config the uart, DMA, vref, timer5 ISR ===========
    PT_setup();

    // === setup system wide interrupts  ====================
    INTEnableSystemMultiVectoredInt();

    PT_INIT(&pt_display);
    PT_INIT(&pt_time);
    PT_INIT(&pt_adc);    

    // schedule the threads
    while(1) {
        PT_SCHEDULE(protothread_display(&pt_display));
        PT_SCHEDULE(protothread_time(&pt_time));
        PT_SCHEDULE(protothread_adc(&pt_adc));
    }
} // main
