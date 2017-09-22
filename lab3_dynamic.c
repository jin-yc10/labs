/*********************************************************************
 *
 *  This example generates a sine wave using a R2R DAC on PORTB 
 *  Refer also to http://tahmidmc.blogspot.com/
 *********************************************************************
 * Bruce Land, Cornell University
 * May 30, 2014
 ********************************************************************/
// all peripheral library includes
#include "config.h"
#include "pt_cornell_1_1.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>
#include <plib.h>
#include <math.h>

#define	SYS_FREQ 40000000
// main ////////////////////////////
static unsigned int sys_time_seconds;
static struct pt pt_dynamic;
char buffer[60];

//#define USE_FIX_POINT
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
#define BALL_RADIUS 3.0f
#define BALL_RADIUS_SQR 9.0f
#define HIT_COUNT 50
#define PI 3.1415926f
#define W 320
#define H 240
#define N 120

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
};
const fix16 FIX16_W = int2fix16(W);
const fix16 FIX16_H = int2fix16(H);
fix16 MIN_DIST_PLUS = int2fix16(4);
fix16 MIN_DIST_MINUS = int2fix16(-4);
fix16 TWOxBALL_RAD_SQR = float2fix16(2.0f*BALL_RADIUS_SQR);
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
    while(1) {
        PT_YIELD_TIME_msec(60-cal_t);
        if( init_cnt < N ) {
			struct ball* b1 = &balls[init_cnt];
			b1->rx = float2fix16( 160.0f);
			b1->ry = float2fix16( 120.0f);
			b1->hit_counter = HIT_COUNT;
			b1->vx = float2fix16(5.0f*sin((float)init_cnt/N*PI));
			b1->vy = float2fix16(5.0f*cos((float)init_cnt/N*PI));
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
//            tft_fillRoundRect(
//                    fix2int16(b1->rx), fix2int16(b1->ry), 
//                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, ILI9340_BLACK);
			if( b1->is_init == 0 ) continue;
			b1->rx += multfix16(beta, b1->vx);
			b1->ry += multfix16(beta, b1->vy);
			if( b1->rx < 0 ) {
				b1->vx = -b1->vx;
				b1->rx = 0;
			} else if( b1->rx > FIX16_W ) {
				b1->vx = -b1->vx;
				b1->rx = FIX16_W;
			}
			if( b1->ry < 0 ) {
				b1->vy = -b1->vy;
				b1->ry = 0;
			} else if( b1->ry > FIX16_H) {
				b1->vy = -b1->vy;
				b1->ry = FIX16_H;
			}
//            tft_fillRoundRect(
//                    fix2int16(b1->rx), fix2int16(b1->ry),
//                    2*BALL_RADIUS, 2*BALL_RADIUS, 2, b1->COLOR);
		}
        cal_t = PT_GET_TIME() - now;
    } // END WHILE(1)
    PT_END(pt);
}
#endif
int main(void) {
    srand(1); // seed of random
    ANSELA = 0; ANSELB = 0;
    PT_setup();
    PT_INIT(&pt_dynamic);
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(1); // FOR HORIZONTAL 320x240
    INTEnableSystemMultiVectoredInt();
	while(1) {		
        PT_SCHEDULE(fn_dynamic(&pt_dynamic));
 	}
	return 0;
}
