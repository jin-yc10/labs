
/*********************************************************************
 *
 *  This example generates a sine wave using a R2R DAC on PORTB 
 *  Refer also to http://tahmidmc.blogspot.com/
 *********************************************************************
 * Bruce Land, Cornell University
 * May 30, 2014
 ********************************************************************/
// all peripheral library includes

//#include <plib.h>
#include <math.h>
#include "config.h"
#include "tft_master.h"
#include "tft_gfx.h"
// Configuration Bit settings
// SYSCLK = 40 MHz 
// PBCLK = 40 MHz
// the sine lookup table
//#include "AllDigits_packed.h"
#include "Sound_bomb.h"
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC
volatile short table[5000];
int table_size ;
#define DAC_config_chan_A 0b0011000000000000

#define	SYS_FREQ 40000000
#include "pt_cornell_1_1.h"
// main ////////////////////////////
static struct pt pt_timer;
int sys_time_seconds = 0;
char buffer[60];
static PT_THREAD (protothread_timer(struct pt *pt))
{
    #define dmaChn 0
    PT_BEGIN(pt);
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Time in seconds since boot\n");
    while(1) {
        // yield time 1 second    
        DmaChnEnable(dmaChn);     
        PT_YIELD_TIME_msec(2000);
        sys_time_seconds++ ;
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        // NEVER exit while
    } // END WHILE(1)
  PT_END(pt);
} // timer thread

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    mT2ClearIntFlag();
}

int main__(void)
{
    int timer_limit ;
    short s;
    SYSTEMConfigPerformance(PBCLK);
    ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;
    INTEnableSystemMultiVectoredInt();
    PT_setup();
    PT_INIT(&pt_timer);
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    //240x320 vertical display
    tft_setRotation(0); // Use tft_setRotation(1) for 320x240
    #define F_OUT 3 //sound is played once a second
    table_size = sizeof(AllDigits) ;
    timer_limit = 40000000/(table_size*F_OUT) ;
    // build the sine lookup table
    // scaled to produce values between 0 and 63 (six bit)
    int i;
    for (i = 0; i < table_size; i++){
        s = (short)(16*AllDigits[i]); //12 bit
        table[i] = DAC_config_chan_A | s; //(short)(0x3000 | s&0x0fff);
    }
//    table[i+1]=0x80000000;
    #define dmaChn 0
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 400);//timer_limit);
    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag
    
    INTEnableSystemMultiVectoredInt();
    
    DmaChnOpen(dmaChn, 0, DMA_OPEN_DEFAULT);
    DmaChnSetTxfer(dmaChn, table, (void*)&SPI2BUF,5000*2, 2, 2);
    DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    DmaChnEnable(dmaChn);   
    
    PPSOutput(2, RPB5, SDO2);
    SpiChnOpen(SPI_CHANNEL2, 
            SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV
            | SPICON_FRMEN | SPICON_FRMPOL
            ,2);

    PPSOutput(4, RPB10, SS2);  
	while(1) {		
        PT_SCHEDULE(protothread_timer(&pt_timer));
 	}
	return 0;
}
