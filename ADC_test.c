
#include "config.h"
// threading library
#include "pt_cornell_1_1.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
////////////////////////////////////

// string buffer
char buffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt  pt_adc ;


// === the fixed point macros ========================================
typedef signed int fix16 ;
#define multfix16(a,b) ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a) ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a) ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b) ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a) (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a) abs(a)

// === Timer Thread =================================================
// update a 1 second tick counter

// === ADC Thread =============================================
// 

static PT_THREAD (protothread_adc(struct pt *pt))
{
    PT_BEGIN(pt);
    static int adc_9;
    static float V, position;
    static fix16 Vfix, ADC_scale ;
    
    ADC_scale = float2fix16(3.3/1023.0); //Vref/(full scale)
            
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(60);
        
        // read the ADC from pin 26 (AN9)
        // read the first buffer position
        adc_9 = ReadADC10(0);   // read the result of channel 9 conversion from the idle buffer
        AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below

        // draw adc and voltage
        tft_fillRoundRect(0,100, 200, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 100);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d %6.3f %d.%03d", adc_9, V, fix2int16(Vfix), fix2int16(Vfix*1000)-fix2int16(Vfix)*1000);
        tft_writeString(buffer);
        
        // convert to voltage
        V = (float)adc_9 * 3.3 / 1023.0 ; // Vref*adc/1023
        position = (float)adc_9 * (3.3/2.8) * 239.0 / 1023.0  ;
        // convert to fixed voltage
        Vfix = multfix16(int2fix16(adc_9), ADC_scale) ;
        

        tft_fillRoundRect(300, 0, 5, (position), 1, ILI9340_BLACK);
        tft_fillRoundRect(300, (position+20), 5, 240-(position+20), 1, ILI9340_BLACK);
        tft_fillRoundRect(300, position, 5, 20, 1, ILI9340_YELLOW);
        // print raw ADC, floating voltage, fixed voltage

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

// === Main  ======================================================
void main(void) { 
  ANSELA = 0; ANSELB = 0; 
  PT_setup();
  INTEnableSystemMultiVectoredInt();

	CloseADC10();	// ensure the ADC is off before setting the configuration
    #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //

	#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
        
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2

	#define PARAM4	ENABLE_AN11_ANA // pin 24

	#define PARAM5	SKIP_SCAN_ALL

	SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN4 
    
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

	EnableADC10(); // Enable the ADC
  ///////////////////////////////////////////////////////
    
  // init the threads
  
  PT_INIT(&pt_adc);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240
  
  // seed random color
  srand(1);
  tft_fillRoundRect(240, (240-60), 5, 60, 1, ILI9340_YELLOW);
  tft_fillRoundRect(240, 0, 5, 60, 1, ILI9340_YELLOW);
  // round-robin scheduler for threads
  while (1){

      PT_SCHEDULE(protothread_adc(&pt_adc));
      }
  } // main

// === end  ======================================================
