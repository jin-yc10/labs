/*********************************************************************
 *
 *  This example generates a sine wave using a R2R DAC on PORTB 
 *  Refer also to http://tahmidmc.blogspot.com/
 *********************************************************************
 * Bruce Land, Cornell University
 * May 30, 2014
 ********************************************************************/
// all peripheral library includes

#include <plib.h>
#include <math.h>
#include "config.h"
// Configuration Bit settings
// SYSCLK = 40 MHz 
// PBCLK = 40 MHz
// the sine lookup table
volatile short sine_table[256];
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC
#define DAC_config_chan_A 0b0011000000000000
// main ////////////////////////////
int main(void)
{
    int sine_table_size ;
    int timer_limit ;
    short s;
    ANSELA = 0; ANSELB = 0; 
//    INTEnableSystemMultiVectoredInt();
    
    #define F_OUT 1000
    sine_table_size = 256 ;
    timer_limit = 40000000/(sine_table_size*F_OUT) ;
    // build the sine lookup table
    // scaled to produce values between 0 and 63 (six bit)
    int i;
    for (i = 0; i < sine_table_size; i++){
        s = (short)(2047.5 * sin((float)i*6.283/(float)sine_table_size) + 2048); //12 bit
        sine_table[i] = DAC_config_chan_A | s; //(short)(0x3000 | s&0x0fff);
    }
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, timer_limit);
    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag
#if 1
    #define dmaChn 0
	DmaChnOpen(dmaChn, 0, DMA_OPEN_AUTO);
	DmaChnSetTxfer(dmaChn, sine_table, (void*)&SPI2BUF, sine_table_size*2, 2, 2);
	DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    DmaChnEnable(dmaChn);
#endif
    PPSOutput(2, RPB5, SDO2);
    SpiChnOpen(SPI_CHANNEL2, 
            SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV
            | SPICON_FRMEN | SPICON_FRMPOL
            ,2);

    PPSOutput(4, RPB10, SS2);    
	while(1)
	{		
 	}
	return 0;
}
