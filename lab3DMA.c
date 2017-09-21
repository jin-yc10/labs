

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
#define	SYS_FREQ 60000000

// the sine lookup table
volatile unsigned char sine_table[256];
volatile unsigned int DAC_data1, DAC_data2 ;// output value
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC

// main ////////////////////////////
int main(void)
{
        int sine_table_size ;
        int timer_limit ;
        unsigned char s;

        // Configure the device for maximum performance but do not change the PBDIV
	// Given the options, this function will change the flash wait states, RAM
	// wait state and enable prefetch cache but will not change the PBDIV.
	// The PBDIV value is already set via the pragma FPBDIV option above..
	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
        
        // frequency settable to better than 2% relative accuracy at low frequency
        // frequency settable to 5% accuracy at highest frequency
        // Useful frequency range 10 Hz to 100KHz
        // >40 db attenuation of  next highest amplitude frequency component below 100 kHz
        // >35 db above 100 KHz
        #define F_OUT 80000
        if (F_OUT <= 5000 ){
            sine_table_size = 256 ;
            timer_limit = SYS_FREQ/(sine_table_size*F_OUT) ;
        }
        else if (F_OUT <= 10000 ){
            sine_table_size = 128 ;
            timer_limit = SYS_FREQ/(sine_table_size*F_OUT) ;
        }
        else if (F_OUT <= 20000 ){
            sine_table_size = 64 ;
            timer_limit = SYS_FREQ/(sine_table_size*F_OUT) ;
        }
        else if (F_OUT <= 100000 ){
            sine_table_size = 32 ;
            timer_limit = SYS_FREQ/(sine_table_size*F_OUT) ;
        }
        else {
            sine_table_size = 16 ;
            timer_limit = SYS_FREQ/(sine_table_size*F_OUT) ;
        }

         // build the sine lookup table
        // scaled to produce values between 0 and 63 (six bit)
        int i;
        for (i = 0; i < sine_table_size; i++){
        //sine_table[i] =(unsigned char) (31.5 * sin((float)i*6.283/(float)sine_table_size) + 32); //
           // s =(unsigned char) (63.5 * sin((float)i*6.283/(float)sine_table_size) + 64); //7 bit
            s =(unsigned char) (63.5 * sin((float)i*6.283/(float)sine_table_size) + 64); //7 bit
            sine_table[i] = (s & 0x3f) | ((s & 0x40)<<1) ;
         }

        // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
        // Uses macro to set timer tick to 2 microSec = 120 cycles 500 kHz DDS
        // peripheral at 60 MHz
        //--------------------
        //------------------
        // 64 LEVEL samples thru external DAC
        // interval=60, 32 samples, 31200 Hz, next harmonic 40 db down
        // interval=20, 32 samples, 90.8 KHz, next harmonic 45 db down
        // interval=600, 32 samples, 3170 Hz, next harmonic 32 db down
        // interval=300, 64 samples, 3170 Hz, next harmonic 38 db down
        // interval=150, 128 samples, 3170 Hz, next harmonic 43  db down
        OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, timer_limit);

        // Open the desired DMA channel.
        #define dmaChn 0
	// We enable the AUTO option, we'll keep repeating the sam transfer over and over.
	DmaChnOpen(dmaChn, 0, DMA_OPEN_AUTO);

	// set the transfer parameters: source & destination address, source & destination size, number of bytes per event
        // Setting the last parameter to one makes the DMA output one byte/interrupt
        DmaChnSetTxfer(dmaChn, sine_table, (void*)&LATB, sine_table_size, 1, 1);

	// set the transfer event control: what event is to start the DMA transfer
        // In this case, timer2
	DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));

	// once we configured the DMA channel we can enable it
	// now it's ready and waiting for an event to occur...
	DmaChnEnable(dmaChn);
	
	

 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // set up DAC on big board
  // timer interrupt //////////////////////////
    // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
    // at 30 MHz PB clock 60 counts is two microsec
    // 400 is 100 ksamples/sec
    // 2000 is 20 ksamp/sec
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 400);

    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

    // SCK2 is pin 26 
    // SDO2 (MOSI) is in PPS output group 2, could be connected to RB5 which is pin 14
    PPSOutput(2, RPB5, SDO2);

    // control CS for DAC
    //mPORTBSetPinsDigitalOut(BIT_4);
    //mPORTBSetBits(BIT_4);

    // divide Fpb by 2, configure the I/O ports. Not using SS in this example
    // 16 bit transfer CKP=1 CKE=1
    // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
    // For any given peripherial, you will need to match these
    // clk divider set to 2 for 20 MHz
    SpiChnOpen(SPI_CHANNEL2, 
            SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL,
            2);
    // SS1 to RPB4 for FRAMED SPI
    PPSOutput(4, RPB10, SS2);
  // end DAC setup
    
        i=0;
	while(1)
	{		
           // toggle a bit for perfromance measure
            //mPORTBToggleBits(BIT_7);
 	}
	return 0;
}
