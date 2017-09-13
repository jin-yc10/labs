#include "config.h"
#include "pt_cornell_1_1.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>

static struct pt pt_timer, pt_capacitance;
int sys_time_seconds ;
char buffer[60];

static int cnt = 0;
void __ISR(_TIMER_1_VECTOR, ipl3) Timer1Handler(void) {
  // use LED to confirm the system is running now
  if( cnt == 0 ) {
    PORTToggleBits(IOPORT_A, BIT_0);
    cnt = 3;
  } else {
    cnt --;
  }
  mT1ClearIntFlag();
}

short capture1, last_capture1=0, capture_period=99;
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl3) IC1Handler(void) {
  // capture1 record the value of capture buffer register
  capture1 = mIC1ReadCapture();
     // capture_period = capture1 - last_capture1 ;
     // last_capture1 = capture1 ;
  // clear the timer interrupt flag
  mIC1ClearIntFlag();
}

float last_measurement = -1.0f;

static PT_THREAD (protothread_measurement(struct pt *pt)) {
  PT_BEGIN(pt);
  while(1) {
    // set RB3(pin7) to zero, to discharge
    PORTSetPinsDigitalOut(IOPORT_B, BIT_3);
    PORTClearBits(IOPORT_B, BIT_3);
    PT_YIELD_TIME_msec(100);

    // Set Pin7 as input comparator 
    mPORTBSetPinsDigitalIn(BIT_3);
    WriteTimer3(0); // set timer to 0
    PT_YIELD_TIME_msec(500);
  }
  PT_END(pt);
}

const float ln_magic = -0.4519851237f; // ln(1-1.2/3.3)
const float R = 693000.0f; // measur first

// TFT_Screen thread, Control the update of TFT. 
static PT_THREAD (protothread_timer(struct pt *pt)) {
  PT_BEGIN(pt);
  tft_setCursor(0, 0);
  tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
  tft_writeString("Time in seconds since boot\n");
  while(1) {
 // yield time 0.2 second
    PT_YIELD_TIME_msec(200);
    sys_time_seconds++;
    tft_fillRoundRect(0, 10, 200, 55, 1, ILI9340_BLACK);// x,y,w,h,radius,color
    tft_setTextColor(ILI9340_YELLOW); 
    tft_setTextSize(2);
 // Check if there is a capacitor or not
    if( capture1 < 5 ) { 
      tft_setCursor(0,25);
      tft_setTextSize(1);
      sprintf(buffer,"No Capacitor Is Present");
      tft_writeString(buffer);
    } else {
 // Calculate value of capacitor from timer counter 
      tft_setCursor(0, 25);
      float dt = capture1*(256.0f/40e6); // second
      float c_val = -(dt)/(R*ln_magic);
      sprintf(buffer,"C = %.01f nf", c_val*1e9);
      tft_writeString(buffer);
    } // END of if(capture1 < 5)
    if(sys_time_seconds % 5 == 0)
      tft_fillRoundRect(110, 150, 20, 20, 10, ILI9340_BLACK);
    else if (sys_time_seconds % 5 == 3)
      tft_fillRoundRect(110, 150, 20, 20, 10, ILI9340_YELLOW);
  } // END of while(1)
  PT_END(pt);
}

#define FOSC 40E6
#define PB_DIV 1
#define PRESCALE 256
#define T1_Tick (FOSC/PB_DIV/PRESCALE/4)

void main() {
  ANSELA = 0; ANSELB = 0; 
  PORTSetPinsDigitalOut(IOPORT_A, BIT_0);

  OpenTimer1( T1_ON|T1_SOURCE_INT|T1_PS_1_256, T1_Tick);
  ConfigIntTimer1(T1_INT_ON|T1_INT_PRIOR_2);

  OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_256, 0xffff);
  OpenCapture1(  IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_TIMER3_SRC | IC_ON );
  ConfigIntCapture1(IC_INT_ON | IC_INT_PRIOR_3 | IC_INT_SUB_PRIOR_3 );

  INTClearFlag(INT_IC1);  // Can be deleted 

  PPSInput(3, IC1, RPB13);   // pin24

  PPSOutput(4, RPB9, C1OUT); // pin18

  CMP1Open(CMP_ENABLE | CMP_OUTPUT_ENABLE | CMP1_NEG_INPUT_IVREF);

  PT_setup();

  INTEnableSystemMultiVectoredInt();

  PT_INIT(&pt_timer);
  PT_INIT(&pt_capacitance);

  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  tft_setRotation(0);

  while(1) {
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_measurement(&pt_capacitance));
  }

  return;
}
