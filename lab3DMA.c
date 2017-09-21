

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

// Configuration Bit settings
// SYSCLK = 40 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF
// Other options are don't care
//
#pragma config FNOSC = FRCPLL, POSCMOD = HS, FPLLIDIV = DIV_2, FPLLMUL = MUL_15, FPBDIV = DIV_1, FPLLODIV = DIV_1
#pragma config FWDTEN = OFF
// turn off alternative functions for port pins B.4 and B.5
#pragma config JTAGEN = OFF, DEBUG = OFF
#pragma config FSOSCEN = OFF

// frequency we're running at

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
    
    #define F_OUT 500
    sine_table_size = 256 ;
    timer_limit = 40000000/(sine_table_size*F_OUT) ;
    // build the sine lookup table
    // scaled to produce values between 0 and 63 (six bit)
    int i;
    for (i = 0; i < sine_table_size; i++){
        //sine_table[i] =(unsigned char) (31.5 * sin((float)i*6.283/(float)sine_table_size) + 32); //
       // s =(unsigned char) (63.5 * sin((float)i*6.283/(float)sine_table_size) + 64); //7 bit
        s = (short)(2047.5 * sin((float)i*6.283/(float)sine_table_size) + 2048); //12 bit
        //sine_table[i] = (s & 0x3f) | ((s & 0x40)<<1) ;
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
//    mPORTBSetPinsDigitalOut(BIT_4);
//    mPORTBSetBits(BIT_4);
    PPSOutput(2, RPB5, SDO2);
    SpiChnOpen(SPI_CHANNEL2, 
            SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV
            | SPICON_FRMEN | SPICON_FRMPOL
            ,2);
    
    // SS1 to RPB4 for FRAMED SPI
    PPSOutput(4, RPB10, SS2);
//    WriteSPI2( DAC_config_chan_A | 4095);    
	while(1)
	{		
        // toggle a bit for perfromance measure
        // mPORTBToggleBits(BIT_7);
//        mPORTBClearBits(BIT_4);
//        while(SPI2STATbits.SPIBUSY) ;
//        mPORTBSetBits(BIT_4);
 	}
	return 0;
}
