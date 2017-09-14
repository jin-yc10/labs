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

// string buffer
char buffer[60];
static int keyinsert[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_key ;

// system 1 second interval tick
int sys_time_seconds ;
int j=0;
int chirp_interval=0;
int number_sysllables=0;
int syllable_duration=0;
int syllable_repeat=0;
int burst_frequency=0;

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
            j++;
            if(j>17) j=0;
            flag =1;
            tft_fillRoundRect(160,200, 200, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        } 
        
        if( i == -1) {
            flag =0;
        }
        
        if (keyinsert[j-1]==10)   // * button   cancel all information
        {
            for(i=0;i<17;i++){ keyinsert[i]=0;}
            j=0;
            tft_fillRoundRect(160,200, 200, 100, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        }
        // draw key number
        
        if( keyinsert[j-1]==11)   // # button    start/Stop
        {
            chirp_interval = keyinsert[0]*1000 + keyinsert[1]*100 + keyinsert[2] *10 + keyinsert[3] *1 ; 
            number_sysllables = keyinsert[4]*100 + keyinsert[5] *10 + keyinsert[6] *1 ; 
            syllable_duration = keyinsert[7]*100 + keyinsert[8] *10 + keyinsert[9] *1 ; 
            syllable_repeat = keyinsert[10]*100 + keyinsert[11] *10 + keyinsert[12] *1 ; 
            burst_frequency = keyinsert[13]*1000 + keyinsert[14] *100 + keyinsert[15] *10+ keyinsert[16] *1 ; 
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
        }
       
        
       //  NEVER exit while
        
      } // END WHILE(1)
  PT_END(pt);
} // keypad thread

// === Main  ======================================================
void main(void) {
 SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; CM1CON = 0; CM2CON = 0;

  // === config threads ==========
  // turns OFF UART support and debugger pin
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
//  PT_INIT(&pt_color);
//  PT_INIT(&pt_anim);
  PT_INIT(&pt_key);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  
  
  tft_setCursor(10, 100);
  tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(1);
  tft_writeString("Push * to cancel");
  
  tft_setCursor(10, 120);
  tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(1);
  tft_writeString("Push # to start/stop");
  int i;
  for(i=0;i<5;i++){
    tft_setCursor(10, 200+ i*15);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    if(i==0)tft_writeString("chirp repeat interval");
    if(i==1)tft_writeString("number of syllables");
    if(i==2)tft_writeString("syllable duration");
    if(i==3)tft_writeString("syllable repeat interval");
    if(i==4)tft_writeString("burst frequency");
        }
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
 //     PT_SCHEDULE(protothread_color(&pt_color));
 //     PT_SCHEDULE(protothread_anim(&pt_anim));
      PT_SCHEDULE(protothread_key(&pt_key));
      }
  } // main

// === end  ======================================================

