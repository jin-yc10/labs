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

 int chirp_repeat_interval = 150;
 int num_of_syllables = 2, syll_cnt = 2;
 int syl_duration = 30;
 int syl_rpt_int = 50;
 int burst_freq = 2000;

unsigned int syll_start = 0;

#define ENV_RAMP (1.0f/400.0f)
#define ENV_ZERO 0.0f
#define ENV_FULL 1.0f
float env_val = 0;


enum ux_state {
    select_parameter,
    set_parameter,
    playing,
    test_mode
};

struct parameter {
    const char* name;
    int digits;
    int min_val;
    int max_val;
    int val;
};

struct parameter paras[5] = {
    {"chi_int", 4, 10, 1000, 200},
    {"num_syl", 3,  1, 100,  3},
    {"syl_dur", 3,  5, 100,  30},
    {"syl_int", 3, 10, 100,  50},
    {"bur_frq", 4, 1000, 6000, 2000},
};
static enum ux_state _state = select_parameter;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    // 74 cycles to get to this point from timer event
    mT2ClearIntFlag();
    
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
        env_val = ENV_FULL; // always be full
    } else {
        env_val = ENV_ZERO;
    }
    
    // end SPI transaction from last interrupt cycle
    mPORTBSetBits(BIT_4);
    
    // === Channel A =============
    // CS low to start transaction
    mPORTBClearBits(BIT_4); // start transaction
    WriteSPI2( DAC_config_chan_A | (DAC_data + 2048));
    // main DDS phase
    phase_accum_main += phase_incr_main  ;
    //sine_index = phase_accum_main>>24 ;
    DAC_data = env_val*cos_table[phase_accum_main>>28];
    isr_time = ReadTimer2() ; // - isr_time;
    
    main_cnt++;
    if( main_cnt == chirp_repeat_interval*100 ) {
        // reset the system state
        main_cnt = 0;
        syll_cnt = num_of_syllables;
    }
} // end ISR TIMER2

// string buffer
char buffer[60];
static int keyinsert[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int keyvalue[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_key, pt_key_alt ;

// system 1 second interval tick
int sys_time_seconds ;
int j=0;
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


static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    static int keypad, i, pattern, flag;
    // order is 0 thru 9 then * ==10 and # ==11
    // no press = -1
    // table is decoded to natural digit order (except for * and #)
    // 0x80 for col 1 ; 0x100 for col 2 ; 0x200 for col 3
    // 0x01 for row 1 ; 0x02 for row 2; etc
    static int keytable[12]={0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);    //Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9);    //Set port as input
tft_setCursor(10, 100);
  tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(1);
  tft_writeString("Push * to cancel");
  
  tft_setCursor(10, 120);
  tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(1);
  tft_writeString("Push # to start/stop");
  for(i=0;i<5;i++){
    tft_setCursor(10, 200+ i*15);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    if(i==0)tft_writeString("chirp repeat interval");
    if(i==1)tft_writeString("number of syllables");
    if(i==2)tft_writeString("syllable duration");
    if(i==3)tft_writeString("syllable repeat interval");
    if(i==4)tft_writeString("burst frequency");
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
            for (i=0; i<12; i++){
                if (keytable[i]==keypad) break;
            }
        }
        else i = -1; // no button pushed
        
        if(i != -1 && flag ==0){
            keyinsert[j]= i ;
            if(keyinsert[j]<10){keyvalue[j]=keyinsert[j];}
            j++;
            if(j>16) j=0;
            flag =1;
            tft_fillRoundRect(160,200, 200, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color
    //        if(keyinsert[j]<10){keyvalue[j]=keyinsert[j];}
        } 
        
        if( i == -1) {
            flag =0;
        }
        
        if (keyinsert[j-1]==10)   // * button   cancel all information
        {
            for(i=0;i<17;i++){ keyvalue[i]=0;
            keyinsert[i]=0;}
            j=0;
            tft_fillRoundRect(160,200, 200, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color

        }
        // draw key number
        
       
        
        if( keyinsert[j-1]==11)   // # button    start/Stop
        {
            
            chirp_interval = keyvalue[0]*1000 + keyvalue[1]*100 + keyvalue[2] *10 + keyvalue[3] *1 ; 
            number_sysllables = keyvalue[4]*100 + keyvalue[5] *10 + keyvalue[6] *1 ; 
            syllable_duration = keyvalue[7]*100 + keyvalue[8] *10 + keyvalue[9] *1 ; 
            syllable_repeat = keyvalue[10]*100 + keyvalue[11] *10 + keyvalue[12] *1 ; 
            burst_frequency = keyvalue[13]*1000 + keyvalue[14] *100 + keyvalue[15] *10+ keyvalue[16] *1 ; 
         
           chirp_repeat_interval = chirp_interval;
           num_of_syllables = number_sysllables;
           syll_cnt = number_sysllables;
           syl_duration = syllable_duration;
           syl_rpt_int = syllable_repeat;
           burst_freq = burst_frequency;
           Fs = (100.0/(burst_freq*TM2_DURA));
           phase_incr_main=100.0*two32/Fs;
        }
        
        
        int a=0; 
        int b=0;
        for(i=0;i<17;i++){
            if(i<4){a=0;b=0;}
            else if(i<7){a=1;b=4;}
            else if(i<10){a=2;b=7;}
            else if(i<13){a=3;b=10;}
            else if(i<17){a=4;b=13;}
            tft_setCursor(160 + (i-b)*15, 200 + a*15);
            tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
            if (keyinsert[i]<10)
            {sprintf(buffer,"%d",keyinsert[i]);
             tft_writeString(buffer);}
            if(i==(j-1)){  
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",keyinsert[j]);
            tft_writeString(buffer);
            }
        }
            
        
            tft_fillRoundRect(10,45, 200, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color
            tft_setCursor(20,45);
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",chirp_repeat_interval);
            tft_writeString(buffer);
            
            tft_setCursor(20,65);
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",num_of_syllables);
            tft_writeString(buffer);
            
            tft_setCursor(20,85);
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",syl_duration);
            tft_writeString(buffer);
                    
            tft_setCursor(20,105);
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",syl_rpt_int);
            tft_writeString(buffer);
            
                        
            tft_setCursor(20,125);
            tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
            sprintf(buffer,"%d",burst_freq);
            tft_writeString(buffer);

       
        
       //  NEVER exit while
        
      } // END WHILE(1)
  PT_END(pt);
} // keypad thread



static PT_THREAD (protothread_key_alt(struct pt *pt)) {
    PT_BEGIN(pt);
    static int parameter_id = 0;
    static int digit_id = 0;
    static int flag = 0, digit_need_update = 0;
    
    static int last_key = -1;
    static int keypad, i, pattern, d, left, p, key_scan;;
    static int keytable[12]={0x108, 0x81, 0x101, 0x201, 0x82, 0x102, 0x202, 0x84, 0x104, 0x204, 0x88, 0x208};
    static int key_cnt[12] ={0};
    // init the keypad pins A0-A3 and B7-B9
    // PortA ports as digital outputs
    mPORTASetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 | BIT_3);    //Set port as output
    // PortB as inputs
    mPORTBSetPinsDigitalIn(BIT_7 | BIT_8 | BIT_9);    //Set port as input
    
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
        switch( _state ) {
            case select_parameter:
                if( key_code == 1 ) {
                    tft_fillRoundRect(145,155+parameter_id*20, 
                            100, 4, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                    parameter_id --;
                    if( parameter_id < 0 ) parameter_id = 4;
                } else if( key_code == 7 ) {
                    tft_fillRoundRect(145,155+parameter_id*20, 
                            100, 4, 1, ILI9340_BLACK);// x,y,w,h,radius,color
                    parameter_id ++;
                    if( parameter_id > 4 ) parameter_id = 0;
                } else if( key_code == 4 ) {
                    _state = set_parameter;
                } else if( key_code == 9 ) {
                    _state = playing;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    chirp_interval = paras[0].val;
                    number_sysllables = paras[1].val;
                    syllable_duration = paras[2].val;
                    syllable_repeat = paras[3].val;
                    burst_frequency = paras[4].val;

                    chirp_repeat_interval = chirp_interval;
                    num_of_syllables = number_sysllables;
                    syll_cnt = number_sysllables;
                    syl_duration = syllable_duration;
                    syl_rpt_int = syllable_repeat;
                    burst_freq = burst_frequency;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                } else if( key_scan == 0 ) {
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    _state = test_mode;
                    burst_freq = paras[4].val;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                }
                break;
            case set_parameter:
                for( p=0; p<paras[parameter_id].digits-digit_id-1; p++ ) {
                    power *= 10;
                }
                if( key_code == 3 ) {
                    digit_id --;
                    if( digit_id < 0 ) digit_id = paras[parameter_id].digits-1;
                } else if( key_code == 5 ) {
                    digit_id ++;
                    if( digit_id > paras[parameter_id].digits-1 ) digit_id = 0;
                } else if( key_code == 4 ) {
                    _state = select_parameter;
                    digit_id = 0;
                } else if( key_code == 1 ) {
                    if( paras[parameter_id].val + power <= paras[parameter_id].max_val ) {
                        paras[parameter_id].val += power;
                        digit_need_update = 1;
                    }
                } else if( key_code == 7 ) {
                    if( paras[parameter_id].val - power >= paras[parameter_id].min_val ) {
                        paras[parameter_id].val -= power;
                        digit_need_update = 1;
                    }
                } else if( key_scan == 0 ) {
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                    _state = test_mode;
                    burst_freq = TEST_MODE_FREQ;
                    Fs = (100.0/(burst_freq*TM2_DURA));
                    phase_incr_main=100.0*two32/Fs;
                }
                break;
            case playing:
                if( key_code == 11 ) {
                    _state = select_parameter;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                } else if( key_scan == 0 ) {
                    _state = test_mode;
                    tft_fillRoundRect(50, 40, 100, 20, 1, ILI9340_BLACK);
                }
                break;
            case test_mode:
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
        
//        tft_fillRoundRect(50, 70, 140, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
//        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(2);
//        tft_setCursor(50, 70);
//        sprintf(buffer, "key=%d@%d %d %d",last_key,key_cnt[last_key],parameter_id,digit_id); 
//        tft_writeString(buffer);
        
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

  // === config threads ==========
  // turns OFF UART support and debugger pin
  PT_setup();

  // === setup system wide interrupts  ========

    
   
  // init the threads
  PT_INIT(&pt_timer);
//  PT_INIT(&pt_color);
//  PT_INIT(&pt_anim);
//  PT_INIT(&pt_key);
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
  
//  
  
  // round-robin scheduler for threads
  
  
    INTEnableSystemMultiVectoredInt();
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, PERIOD);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();
    
    
    PPSOutput(2, RPB5, SDO2);
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
    
//    int i;
    for (i = 0; i < sine_table_size; i++){
         cos_table[i] = (int)(2047*cos((float)i*6.283/(float)sine_table_size));
         sin_table[i] = (int)(2047*sin((float)i*6.283/(float)sine_table_size));
    }
    
    
    
  while (1){
        PT_SCHEDULE(protothread_timer(&pt_timer));
 //     PT_SCHEDULE(protothread_color(&pt_color));
 //     PT_SCHEDULE(protothread_anim(&pt_anim));
//      PT_SCHEDULE(protothread_key(&pt_key));
        PT_SCHEDULE(protothread_key_alt(&pt_key_alt));
      }
  } // main

// === end  ======================================================

