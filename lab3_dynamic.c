/*********************************************************************
 *
 *  This example generates a sine wave using a R2R DAC on PORTB 
 *  Refer also to http://tahmidmc.blogspot.com/
 *********************************************************************
 * Bruce Land, Cornell University
 * May 30, 2014
 ********************************************************************/
// all peripheral library includes
#define _SUPPRESS_PLIB_WARNING 1
#include "config.h"
#include "pt_cornell_1_1.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>
#include <plib.h>
#include <math.h>
#include "Sound_bomb.h"

#define	SYS_FREQ 40000000
// main ////////////////////////////
static unsigned int sys_time_seconds;
static struct pt pt_dynamic, pt_adc;
char buffer[60];

// Sound
#define dmaChn 0
#define F_OUT 3
volatile short table[5000];
unsigned int table_size;
unsigned int timer_limit;
#define DAC_config_chan_A 0b0011000000000000
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    // Just fot correction
    mT2ClearIntFlag();
}

#define USE_FIX_POINT
// ====   FIX   ====
#ifdef USE_FIX_POINT
typedef signed int fix16 ;
#define multfix16(a,b)  ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) 
#define float2fix16(a)  ((fix16)((a)*65536.0))
#define fix2float16(a)  ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b)   ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a)    (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a)     abs(a)
#endif

// ==== DYNAMIC ====
#define BALL_RADIUS 2.0f
#define BALL_RADIUS_SQR 4.0f
#define HIT_COUNT 1
#define PI 3.1415926f
#define W 320
#define H 240
#define N 30

static int score = 0;

#ifndef USE_FIX_POINT
struct ball {
	float rx, ry;
	float vx, vy;
	unsigned int hit_counter;
    unsigned short COLOR;
    unsigned short is_init;
};
#else
struct ball {
	fix16 rx, ry;
	fix16 vx, vy;
    unsigned short COLOR;
	unsigned char hit_counter;
    unsigned char is_init;
    unsigned char needs_recycle;
};
const fix16 FIX16_W = int2fix16(W);
const fix16 FIX16_H = int2fix16(H);
fix16 MIN_DIST_PLUS = int2fix16(4);
fix16 MIN_DIST_MINUS = int2fix16(-4);
fix16 TWOxBALL_RAD_SQR = float2fix16(2.0f*BALL_RADIUS_SQR);
fix16 BOARD_X = int2fix16(80);
fix16 BOARD_Y_TOP = int2fix16(60);
fix16 BOARD_Y_BOT_TOP = int2fix16(180);
fix16 BOARD_Y_BOT_BOT = int2fix16(240);
fix16 DRAG = float2fix16(0.999f);
#endif

struct ball balls[N];
//void tft_fillRoundRect(short x, short y, short w,
//				 short h, short r, unsigned short color)

unsigned short color_table[7] = {ILI9340_BLUE,ILI9340_RED,
ILI9340_GREEN,ILI9340_CYAN,ILI9340_MAGENTA,
ILI9340_YELLOW,ILI9340_WHITE};
static unsigned short rand_color() {
    char x = (rand()%7);
    return color_table[x];
}

#define PADDLE_X 20
#define PADDLE_WIDTH 60
static unsigned int paddle_y = 90;

#ifndef USE_FIX_POINT
static PT_THREAD (fn_dynamic(struct pt *pt)) {
    PT_BEGIN(pt);
    static unsigned int init_cnt = 0;
    static unsigned int last_time = 0;
    static int i, j;
    static unsigned int now, dt = 0, cal_t = 0;
    while(1) {
        PT_YIELD_TIME_msec(60-cal_t);
        if( init_cnt < N ) {
			struct ball* b1 = &balls[init_cnt];
			b1->rx = 0;
			b1->ry = H/2;
			b1->hit_counter = init_cnt;
			b1->vx = 5.0f*sin((float)init_cnt/N*PI);
			b1->vy = 5.0f*cos((float)init_cnt/N*PI);
			b1->is_init = 1;
            b1->COLOR = rand_color();
			init_cnt++;
		}        
        now = PT_GET_TIME();
        dt = now - last_time; last_time = now;
        tft_setCursor(0, 0);
        tft_fillRoundRect(0 ,0, 80, 40, 1, ILI9340_BLACK);
        tft_setTextColor(ILI9340_YELLOW);  tft_setTextSize(2);
        sprintf(buffer, "%d, %d", dt, cal_t);
        tft_writeString(buffer);
        for( i=0; i<N; i++) {
			struct ball* b1 = &balls[i];
			for( j=i; j<N; j++) {
				struct ball* b2 = &balls[j];
				float dx = b1->rx - b2->rx;
				float dy = b1->ry - b2->ry;
				if( dx < 4.0f && -4.0f < dx && dy < 4.0f && -4.0f < dy ) {
					float mod_rij = dx*dx+dy*dy;
					if( mod_rij == 0.0f && b1->hit_counter == 0) {
						// avoid divided by zero
						float swp;
						swp = b1->vx;
						b1->vx = b2->vx;
						b2->vx = swp;
						swp = b1->vy;
						b1->vy = b2->vy;
						b2->vy = swp;
					}
					else if( mod_rij < 2*BALL_RADIUS_SQR && b1->hit_counter == 0 ) {
						float vij_x = b1->vx - b2->vx;
						float vij_y = b1->vy - b2->vy;
						float rij_times_vij = dx*vij_x+dy*vij_y;
						float alpha = rij_times_vij/mod_rij;
						float dvix = alpha*(-dx);
						float dviy = alpha*(-dy);
						b1->vx += dvix;
						b1->vy += dviy;
						b2->vx -= dvix;
						b2->vy -= dviy;
						b1->hit_counter = HIT_COUNT;
						b2->hit_counter = HIT_COUNT;
					} else if( b1->hit_counter > 0 ) {
						b1->hit_counter --;
					}
				}
			}
		}
        for( i=0; i<N; i++) {
			struct ball* b1 = &balls[i];
//            tft_fillRoundRect(b1->rx, b1->ry, 
//                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, ILI9340_BLACK);
			if( b1->is_init == 0 ) continue;
			b1->rx += 0.05f*dt*b1->vx;
			b1->ry += 0.05f*dt*b1->vy;
			if( b1->rx < 0 ) {
				b1->vx = -b1->vx;
				b1->rx = 0.0f;
			} else if( b1->rx > W ) {
				b1->vx = -b1->vx;
				b1->rx = W;
			}
			if( b1->ry < 0 ) {
				b1->vy = -b1->vy;
				b1->ry = 0;
			} else if( b1->ry > H) {
				b1->vy = -b1->vy;
				b1->ry = H;
			}
//            tft_fillRoundRect(b1->rx, b1->ry, 
//                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, b1->COLOR);
		}
        cal_t = PT_GET_TIME() - now;
    } // END WHILE(1)
    PT_END(pt);
}
#else
static PT_THREAD (fn_dynamic(struct pt *pt)) {
    PT_BEGIN(pt);
    static unsigned int init_cnt = 0;
    static unsigned int last_time = 0;
    static int i, j;
    static unsigned int now, dt = 0, cal_t = 0;
    
    for( i=0; i<N; i++ ) {
        struct ball* b1 = &balls[i];
        b1->needs_recycle = 1;
        b1->COLOR = rand_color();
    }
    
    while(1) {
        PT_YIELD_TIME_msec(60-cal_t);
//        if( init_cnt < N ) {
//			struct ball* b1 = &balls[init_cnt];
//			b1->rx = float2fix16( 320.0f);
//			b1->ry = float2fix16( 120.0f);
//			b1->hit_counter = HIT_COUNT;
//			b1->vx = float2fix16(-5.0f*sin((float)init_cnt/N*PI));
//			b1->vy = float2fix16(5.0f*cos((float)init_cnt/N*PI));
//			b1->is_init = 1;
//            b1->COLOR = rand_color();
//			init_cnt++;
//		}
        for( i=0; i<N; i++ ) {
            struct ball* b1 = &balls[i];
            if ( b1->needs_recycle ) {
                b1->needs_recycle = 0;
                b1->rx = float2fix16( 320.0f);
    			b1->ry = float2fix16( 120.0f);
                b1->hit_counter = HIT_COUNT;
                b1->vx = float2fix16(-5.0f*sin((float)init_cnt/N*PI));
                b1->vy = float2fix16(5.0f*cos((float)init_cnt/N*PI));
                b1->is_init = 1;
                init_cnt++;
                if( init_cnt > N-1 ) {
                    init_cnt = 0;
                }
                break;
            }
        }
        now = PT_GET_TIME();
        dt = now - last_time; last_time = now;
        tft_setCursor(0, 0);
        tft_fillRoundRect(0 ,0, 120, 20, 1, ILI9340_BLACK);
        tft_setTextColor(ILI9340_YELLOW);  tft_setTextSize(2);
        sprintf(buffer, "%d,%d,%d", dt, cal_t, score);
        tft_writeString(buffer);
#if 1
        for( i=0; i<N; i++) {
			struct ball* b1 = &balls[i];
			for( j=i; j<N; j++) {
				struct ball* b2 = &balls[j];
				fix16 dx = b1->rx - b2->rx;
				fix16 dy = b1->ry - b2->ry;
				if( dx < MIN_DIST_PLUS && MIN_DIST_MINUS < dx && 
                    dy < MIN_DIST_PLUS && MIN_DIST_MINUS < dy ) {
					fix16 mod_rij = multfix16(dx,dx)+multfix16(dy,dy);
					if( mod_rij == 0 && b1->hit_counter == 0) {
						// avoid divided by zero
						fix16 swp;
						swp = b1->vx;
						b1->vx = b2->vx;
						b2->vx = swp;
						swp = b1->vy;
						b1->vy = b2->vy;
						b2->vy = swp;
					}
					else if( mod_rij < TWOxBALL_RAD_SQR && b1->hit_counter == 0 ) {
						fix16 vij_x = b1->vx - b2->vx;
						fix16 vij_y = b1->vy - b2->vy;
						fix16 rij_times_vij = multfix16(dx,vij_x)+multfix16(dy,vij_y);
						fix16 alpha = divfix16(rij_times_vij,mod_rij);
						fix16 minus_dvix = multfix16(alpha,dx);
						fix16 minus_dviy = multfix16(alpha,dy);
						b1->vx -= minus_dvix;
						b1->vy -= minus_dviy;
						b2->vx += minus_dvix;
						b2->vy += minus_dviy;
						b1->hit_counter = HIT_COUNT;
						b2->hit_counter = HIT_COUNT;
					} else if( b1->hit_counter > 0 ) {
						b1->hit_counter --;
					}
				}
			}
		}
#endif
        fix16 beta = float2fix16(0.01f*dt);
        for( i=0; i<N; i++) {
			struct ball* b1 = &balls[i];
            tft_fillRoundRect(
                    fix2int16(b1->rx), fix2int16(b1->ry), 
                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, ILI9340_BLACK);
			if( b1->is_init == 0 ) continue;
            if(BOARD_X > b1->rx && BOARD_X < b1->rx + multfix16(beta, b1->vx) &&
               (  0 < b1->ry && b1->ry < BOARD_Y_TOP ||
                  BOARD_Y_BOT_TOP < b1->ry && b1->ry < BOARD_Y_BOT_BOT )) {
                b1->needs_recycle = 1;
                tft_fillRoundRect(
                    fix2int16(b1->rx), fix2int16(b1->ry), 
                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, ILI9340_BLACK);
                DmaChnEnable(dmaChn); 
                score++;
            } else if(( BOARD_X < b1->rx && 
                        BOARD_X > b1->rx + multfix16(beta, b1->vx) &&
                    (  0 < b1->ry && b1->ry < BOARD_Y_TOP ||
                    BOARD_Y_BOT_TOP < b1->ry && b1->ry < BOARD_Y_BOT_BOT )) 
                    ||
                    int2fix16(PADDLE_X) < b1->rx && 
                    int2fix16(PADDLE_X) > b1->rx + multfix16(beta, b1->vx) &&
                    int2fix16(paddle_y) < b1->ry && 
                    b1->ry < int2fix16(paddle_y+PADDLE_WIDTH)  ) {
                b1->vx = -b1->vx;
                b1->vy = multfix16(DRAG, b1->vy);
                b1->rx += multfix16(beta, b1->vx);
                b1->ry += multfix16(beta, b1->vy);
                tft_fillRoundRect(
                    fix2int16(b1->rx), fix2int16(b1->ry),
                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, b1->COLOR);
            } else {
                b1->rx += multfix16(beta, b1->vx);
                b1->ry += multfix16(beta, b1->vy);
                if( b1->rx < 0 ) {
                    // DECREASE 1 SCORE
                    score --;
                    b1->needs_recycle = 1;
                    tft_fillRoundRect(
                    fix2int16(b1->rx), fix2int16(b1->ry), 
                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, ILI9340_BLACK);
                    continue;
                } else if( b1->rx > FIX16_W ) {
                    b1->vx = -b1->vx;
                    b1->vy = multfix16(DRAG, b1->vy);
                    b1->rx = FIX16_W;
                    b1->vy = multfix16(DRAG, b1->vy);
                }
                if( b1->ry < 0 ) {
                    b1->vy = -b1->vy;
                    b1->vx = multfix16(DRAG, b1->vx);
                    b1->ry = 0;
                } else if( b1->ry > FIX16_H) {
                    b1->vy = -b1->vy;
                    b1->vx = multfix16(DRAG, b1->vx);
                    b1->ry = FIX16_H;
                }
                tft_fillRoundRect(
                    fix2int16(b1->rx), fix2int16(b1->ry),
                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, b1->COLOR);
            }
            
		} // END RENDER BALLS
        tft_drawLine(80,0,80,60,ILI9340_GREEN);
        tft_drawLine(80,180,80,240,ILI9340_GREEN);
        tft_drawLine(20,0,20,240,ILI9340_BLACK);
        tft_drawLine(20,paddle_y,20,paddle_y+PADDLE_WIDTH,ILI9340_RED);
        cal_t = PT_GET_TIME() - now;
    } // END WHILE(1)
    PT_END(pt);
}
#endif

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
//        tft_fillRoundRect(0,100, 200, 15, 1, ILI9340_BLACK);// x,y,w,h,radius,color
//        tft_setCursor(0, 100);
//        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
//        sprintf(buffer,"%d %6.3f %d.%03d", adc_9, V, fix2int16(Vfix), fix2int16(Vfix*1000)-fix2int16(Vfix)*1000);
//        tft_writeString(buffer);
        
        // convert to voltage
        V = (float)adc_9 * 3.3 / 1023.0 ; // Vref*adc/1023
        position = (float)adc_9 * (3.3/2.8) * 239.0 / 1023.0  ;
        // convert to fixed voltage
        Vfix = multfix16(int2fix16(adc_9), ADC_scale) ;
        paddle_y = (unsigned int)position;

//        tft_fillRoundRect(300, 0, 5, (position), 1, ILI9340_BLACK);
//        tft_fillRoundRect(300, (position+20), 5, 240-(position+20), 1, ILI9340_BLACK);
//        tft_fillRoundRect(300, position, 5, 20, 1, ILI9340_YELLOW);
        // print raw ADC, floating voltage, fixed voltage

        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

int main(void) {
    srand(1); // seed of random
    ANSELA = 0; ANSELB = 0;
    table_size = sizeof(AllDigits) ;
    timer_limit = 40000000/(table_size*F_OUT) ;

    int i;
    short s;
    for (i = 0; i < table_size; i++){
        s = (short)(16*AllDigits[i]); //12 bit
        table[i] = DAC_config_chan_A | s; //(short)(0x3000 | s&0x0fff);
    }
    
    PT_setup();
    PT_INIT(&pt_dynamic);
    PT_INIT(&pt_adc);
    
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(1); // FOR HORIZONTAL 320x240
    
    CloseADC10();	// ensure the ADC is off before setting the configuration
    #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //
	#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2
	#define PARAM4	ENABLE_AN11_ANA // pin 24
	#define PARAM5	SKIP_SCAN_ALL
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN4 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above
	EnableADC10(); // Enable the ADC
    
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 400);//timer_limit);
    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag
    DmaChnOpen(dmaChn, 0, DMA_OPEN_DEFAULT);
    DmaChnSetTxfer(dmaChn, table, (void*)&SPI2BUF,5000*2, 2, 2);
    DmaChnSetEventControl(dmaChn, DMA_EV_START_IRQ(_TIMER_2_IRQ));
    DmaChnEnable(dmaChn);   
    
    PPSOutput(2, RPB5, SDO2);
    PPSOutput(4, RPB10, SS2); 
    SpiChnOpen(SPI_CHANNEL2, 
            SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV
            | SPICON_FRMEN | SPICON_FRMPOL
            ,2);

    INTEnableSystemMultiVectoredInt();
	while(1) {		
        PT_SCHEDULE(fn_dynamic(&pt_dynamic));
        PT_SCHEDULE(protothread_adc(&pt_adc));
 	}
	return 0;
}
