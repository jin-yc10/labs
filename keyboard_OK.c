/*
 * File:        TFT_keypad_BRL4.c
 * Author:      Bruce Land
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

// graphics libraries
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"

#define _SUPPRESS_PLIB_WARNING

// need for rand function
#include <stdlib.h>

// threading library
#include <plib.h>
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_1.h"

/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000
#define PRESCALE 1          //TODO: CHANGE IF WE CHANGE THE PRESCALE
#define PERIOD 400.0f
#define TM2_FREQ (SYS_FREQ/PERIOD)
#define TM2_DURA 1.0/TM2_FREQ
#define freq 1000.0
//#define Fs (100.0/(freq*TM2_DURA))
#define two32 4294967296.0 // 2^32 
#include <math.h>

volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 15 MHz DAC clock

// === 16:16 fixed point macros ==========================================
typedef signed int fix16 ;
#define multfix16(a,b) ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a) ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a) ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b) ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a) (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a) abs(a)
#define onefix16 0x00010000 // int2fix16(1)

#define sine_table_size 16
volatile fix16 sin_table[sine_table_size], cos_table[sine_table_size] ;



//== Timer 2 interrupt handler ===========================================
// actual scaled DAC 
volatile  int DAC_data = 2047, quadrature_data;
// ADC output
volatile  int ADC_net;	// conversion result as read from result buffer
// the DDS units:
 int Fs = (100.0/(2000*TM2_DURA));
volatile unsigned int phase_accum_main, phase_incr_main=100.0*two32/(100.0/(2000*TM2_DURA)) ;//
// profiling of ISR
volatile int isr_time;
volatile int I_signal, Q_signal ;
//volatile int sine_index ;

// averaging the signal
int avg_size = 50000;
volatile int avg_count;
// accumulators
volatile long long I_avg, Q_avg;
// and the final averages
float I_out, Q_out, I_out_base, Q_out_base ;

unsigned int main_cnt = 0;

// Five Parameters
int chirp_repeat_interval = 150;
int num_of_syllables = 2, syll_cnt = 2;
int syl_duration = 30;
int syl_rpt_int = 50;
int burst_freq = 2000;

// Counter for one single syllable
unsigned int syll_start = 0;

// Variable for envelope
#define ENV_RAMP (1.0f/400.0f)
#define ENV_ZERO 0.0f
#define ENV_FULL 1.0f
float env_val = 0;

// User Interface States
enum ux_state {
    select_parameter,
    set_parameter,
    playing,
    test_mode
};

// Struct to organize parameters
struct parameter {
    const char* name;
    int digits;
    int min_val;
    int max_val;
    int val;
};

// Predefined information about states
struct parameter paras[5] = {
    {"chi_int", 4, 10, 1000, 200},
    {"num_syl", 3,  1, 100,  3},
    {"syl_dur", 3,  5, 100,  30},
    {"syl_int", 3, 10, 100,  50},
    {"bur_frq", 4, 1000, 6000, 2000},
};

// The global state variable
static enum ux_state _state = select_parameter;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    mT2ClearIntFlag();
    
    // If the state is playing, we generate the signal as instruction
    if( _state == playing ) {
        if(main_cnt == 100*syl_rpt_int*(num_of_syllables-syll_cnt) && syll_cnt != 0 ) {
             // just start the next syllable
             phase_accum_main = 0; // sine wave should start from zero
             syll_start = main_cnt;
             syll_cnt--;
        }

        if(main_cnt-syll_start < 400) {
            // up edge
            env_val = ENV_RAMP*(main_cnt-syll_start);
        } else if( main_cnt-syll_start < 100*syl_duration) {
            // full
            env_val = ENV_FULL;
        } else if( main_cnt-syll_start < 100*syl_duration+400 ) {
            // down edge
            env_val = ENV_RAMP*(100*syl_duration+400-(main_cnt-syll_start));
        } else {
            // between two syllables
            env_val = ENV_ZERO;
        } 
    } else if( _state == test_mode ) {
        env_val = ENV_FULL; // always be full, we generate a simple sine wave
    } else {
        // We are neither playing nor testing, so we mute the output signal
        env_val = ENV_ZERO;
    }
    
    // end SPI transaction from last interrupt cycle
    mPORTBSetBits(BIT_4);
    // CS low to start transaction
    mPORTBClearBits(BIT_4); // start transaction
    // Use Channel A as output
    WriteSPI2( DAC_config_chan_A | (DAC_data + 2048));
    // main DDS phase
    phase_accum_main += phase_incr_main;
    // Pick a phase value from table and modulate the signal with envelope
    DAC_data = env_val*cos_table[phase_accum_main>>28];
    // increase the main counter
    main_cnt++;
    // This means we finish a whole chirp
    if( main_cnt == chirp_repeat_interval*100 ) {
        // reset the system state
        main_cnt = 0;
        syll_cnt = num_of_syllables;
    }
} // end ISR TIMER2

// string buffer
char buffer[60];

// Used for debouncing
static int keyinsert[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int keyvalue[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static struct pt pt_timer, pt_color, pt_anim, pt_key, pt_key_alt ;

// system 1 second interval tick
int sys_time_seconds;
int j=0;

// Actually similar to the previous parameters, this is for moving data around
int chirp_interval=0;
int number_sysllables=0;
int syllable_duration=0;
int syllable_repeat=0;
int burst_frequency=0;

#define TEST_MODE_FREQ 1000

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Time in seconds since boot\n");
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;

        // draw sys_time
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        // NEVER exit while
    } // END WHILE(1)
  PT_END(pt);
} // timer thread

// This is the thread handles the keyscan and user interface
static PT_THREAD (protothread_key_alt(struct pt *pt)) {
    PT_BEGIN(pt);
    // Which parameter do we select
    static int parameter_id = 0;
    // Which number are we editing
    static int digit_id = 0;
    // Some flags
    static int flag = 0, digit_need_update = 0;
    static int last_key = -1;
    static int keypad, i, pattern, d, left, p, key_scan;;
    // Scan code
    static int keytable[12]={0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};
    static int key_cnt[12] ={0};
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);    //Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9);    //Set port as input
    
    // We initally print the numbers
    for( i=0; i<5; i++ ) {
        int digits_cnt = paras[i].digits;
        int t = paras[i].val;
        sprintf(buffer, "%*d", digits_cnt, t);
        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
        tft_setCursor(140, 140+i*20);
        tft_writeString(buffer);                 
    }
    
    while(1) {
        // read each row sequentially
        mPORTAClearBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);
        pattern = 1; mPORTASetBits(pattern);
        
        // yield time
        PT_YIELD_TIME_msec(30);
        for (i=0; i<4; i++) {
            keypad  = mPORTBReadBits(BIT_7 | BIT_8 | BIT_9);
            if(keypad!=0) {keypad |= pattern ; break;}
            mPORTAClearBits(pattern);
            pattern <<= 1;
            mPORTASetBits(pattern);
        }
        // search for keycode
        if (keypad > 0){ // then button is pushed
            for (key_scan=0; key_scan<12; key_scan++){
                if (keytable[key_scan]==keypad) break;
            }
        }
        else key_scan = -1; // no button pushed
        
        // Do the decounce and transform the keycode to a different scheme
        int key_code = -1;
        if(key_scan != -1 && flag == 0) {
            flag = 1;
            if( key_scan == 0) {
                key_code = 10;
            } else if( key_scan == 10 ) {
                key_code = 9;
            } else if( key_scan == 11 ) {
                key_code = 11;
            } else {
                key_code = key_scan-1;    
            }        
            if( key_code == -1 ) {
                key_code = 0;
            }
            last_key = key_code;
            key_cnt[key_code]++;
        }
        if( key_scan == -1 ) {
            flag = 0;
        }
        
        int power = 1;
        // User interface FSM
        // Here keycode is changed
        //  0  1  2
        //  3  4  5
        //  6  7  8
        //  9 10 11
        switch( _state ) {
            // State for select parameter
            case select_parameter:
                if( key_code == 1 ) { // 1 for up
                    // clear the digit bar
                    tft_fillRoundRect(145,155+parameter_id*20, 
                            100, 4, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                    parameter_id --;
                    if( parameter_id < 0 ) parameter_id = 4;
                } else if( key_code == 7 ) { // 7 for down
                    tft_fillRoundRect(145,155+parameter_id*20, 
                            100, 4, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                    parameter_id ++;
                    if( parameter_id > 4 ) parameter_id = 0;
                } else if( key_code == 4 ) { // 4 for confirm
                    _state = set_parameter;
                } else if( key_code == 9 ) { // 9 for playing
                    _state = playing;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    chirp_interval = paras[0].val;
                    number_sysllables = paras[1].val;
                    syllable_duration = paras[2].val;
                    syllable_repeat = paras[3].val;
                    burst_frequency = paras[4].val;

                    // Copy the values
                    chirp_repeat_interval = chirp_interval;
                    num_of_syllables = number_sysllables;
                    syll_cnt = number_sysllables;
                    syl_duration = syllable_duration;
                    syl_rpt_int = syllable_repeat;
                    burst_freq = burst_frequency;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                } else if( key_scan == 0 ) {
                    // Every time we press the key 0, we enter the test_mode
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    _state = test_mode;
                    burst_freq = paras[4].val;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                }
                break;
            case set_parameter:
                // we first calculate the power of 10
                for( p=0; p<paras[parameter_id].digits-digit_id-1; p++ ) {
                    power *= 10;
                }
                if( key_code == 3 ) {
                    // 3 for left
                    digit_id --;
                    if( digit_id < 0 ) digit_id = paras[parameter_id].digits-1;
                } else if( key_code == 5 ) {
                    // 5 for right
                    digit_id ++;
                    if( digit_id > paras[parameter_id].digits-1 ) digit_id = 0;
                } else if( key_code == 4 ) {
                    // 4 for go back to select_parameter mode
                    _state = select_parameter;
                    digit_id = 0;
                } else if( key_code == 1 ) {
                    // 1 for increase the specific power, we will check if the value exceed the maximum value
                    if( paras[parameter_id].val + power <= paras[parameter_id].max_val ) {
                        paras[parameter_id].val += power;
                        digit_need_update = 1;
                    }
                } else if( key_code == 7 ) {
                    // 7 for decrease
                    if( paras[parameter_id].val - power >= paras[parameter_id].min_val ) {
                        paras[parameter_id].val -= power;
                        digit_need_update = 1;
                    }
                } else if( key_scan == 0 ) {
                    // same logic as the previous state
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    _state = test_mode;
                    burst_freq = paras[4].val;;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                }
                break;
            case playing:
                if( key_code == 11 ) {
                    // press # to stop
                    _state = select_parameter;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                } else if( key_scan == 0 ) {
                    // press 0 to enter test_mode
                    _state = test_mode;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                }
                break;
            case test_mode:
                // if we are in test_mode, we will detect whether key 0 is released
                if( key_scan != 0 ) {
                    // jump back to normal mode asap
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    _state = select_parameter;
                }
                break;
            default:
                break;
        }
        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
        tft_setCursor(50, 40);
        // print the state
        if( _state == test_mode ) {
            sprintf(buffer, "testing");
            tft_writeString(buffer);
            continue;
        } else{    
            if( _state == playing ) {
                sprintf(buffer, "playing");
            } else {
                sprintf(buffer, "stop");
            }
            tft_writeString(buffer);
        }
        
        // print the parameters' name
        for( i=0; i<5; i++ ) {
            if ( i == parameter_id ) {
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
            } else {
                tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            }
            tft_setCursor(20, 140+i*20);
            sprintf(buffer, "%s", paras[i].name);
            tft_writeString(buffer);
        }
        
        // display digits
        for( i=0; i<5; i++ ) {
            if( _state == set_parameter && parameter_id == i ) {                
                if(digit_need_update == 1) {
                    tft_fillRoundRect(140,140+i*20, 100, 20, 1, ILI9340_BLACK);// x,y,w,h,radius,color    
                    int digits_cnt = paras[i].digits;
                    int t = paras[i].val;
                    sprintf(buffer, "%*d", digits_cnt, t);
                    tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
                    tft_setCursor(140, 140+i*20);
                    tft_writeString(buffer);                 
                }
                tft_fillRoundRect(145,155+i*20, 100, 4, 1, ILI9340_BLACK);// x,y,w,h,radius,color    
                tft_fillRoundRect(145+digit_id*10,155+i*20, 10, 4, 1, ILI9340_RED);// x,y,w,h,radius,color
            }
        }
    }
    PT_END(pt);
}

// === Main  ======================================================
void main(void) {
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    PT_setup();

    PT_INIT(&pt_timer);
    PT_INIT(&pt_key_alt);

    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    //240x320 vertical display
    tft_setRotation(0); // Use tft_setRotation(1) for 320x240

    // seed random color
    srand(1);

    int i;

    // Enable timer2
    INTEnableSystemMultiVectoredInt();
    // PERIOD is 400 se our interrupt will triggered every 10us
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, PERIOD);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();

    // Enable SPI2    
    PPSOutput(2, RPB5, SDO2);
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
    
    // Generate the sine and cosine table
    for (i = 0; i < sine_table_size; i++){
         cos_table[i] = (int)(2047*cos((float)i*6.283/(float)sine_table_size));
         sin_table[i] = (int)(2047*sin((float)i*6.283/(float)sine_table_size));
    }
       
    // Start scheduling
    while (1) {
        PT_SCHEDULE(protothread_timer(&pt_timer));
        PT_SCHEDULE(protothread_key_alt(&pt_key_alt));
    }
} // main

// === end  ======================================================

