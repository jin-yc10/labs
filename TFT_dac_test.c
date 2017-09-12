/* 
 * File:   TFT_dac_test.c
 * Author: Lab User
 *
 * Created on September 7, 2017, 6:16 PM
 */

#define _SUPPRESS_PLIB_WARNING
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>

// threading library
#include <plib.h>
// config.h sets 40 MHz
#define	SYS_FREQ 40000000
#include "pt_cornell_1_1.h"
char buffer[60];
#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000

#define PRESCALE 1          //TODO: CHANGE IF WE CHANGE THE PRESCALE
#define PERIOD 400.0f
#define SYS_FREQ 4.0e7
#define TM2_FREQ (SYS_FREQ/PERIOD)
#define TM2_DURA 1.0/TM2_FREQ
#define freq 1000.0
#define Fs (100.0/(freq*TM2_DURA))
#define two32 4294967296.0 // 2^32 
#include <math.h>
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
// for 60 MHz PB clock use divide-by-3
volatile int spiClkDiv = 2 ; // 15 MHz DAC clock

// system 1 second interval tick
int sys_time_seconds ;

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
volatile unsigned int phase_accum_main, phase_incr_main=100.0*two32/Fs ;//
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

unsigned int chirp_repeat_interval = 150;
unsigned int num_of_syllables = 2, syll_cnt = 2;
unsigned int syl_duration = 30;
unsigned int syl_rpt_int = 50;
unsigned int burst_freq = 1000;

unsigned int syll_start = 0;

#define ENV_RAMP (1.0f/400.0f)
#define ENV_ZERO 0.0f
#define ENV_FULL 1.0f
float env_val = 0;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    // 74 cycles to get to this point from timer event
    mT2ClearIntFlag();
    
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
        env_val = ENV_RAMP*(100*syl_duration+400-main_cnt);
    } else {
        // between two syllables
        env_val = ENV_ZERO;
    }
    
    // end SPI transaction from last interrupt cycle
    mPORTBSetBits(BIT_4);
    
    // === Channel A =============
    // CS low to start transaction
    mPORTBClearBits(BIT_4); // start transaction
    // test for ready
    // while (TxBufFullSPI2());
    // write to spi2 
    
//    if(DAC_data + 2048 > 4095) {
//        DAC_data =- 2047;
//    } else if (DAC_data + 2048 < 0) {
//        DAC_data = -2048;
//    }
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


/*
 * 
 */
int main(int argc, char** argv) {
    PT_setup();
    INTEnableSystemMultiVectoredInt();
    
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, PERIOD);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();
    
    PPSOutput(2, RPB5, SDO2);
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
    
    int i;
    for (i = 0; i < sine_table_size; i++){
         cos_table[i] = (int)(2047*cos((float)i*6.283/(float)sine_table_size));
         sin_table[i] = (int)(2047*sin((float)i*6.283/(float)sine_table_size));
    }
    
    while(1) {}
    
    return (EXIT_SUCCESS);
}

