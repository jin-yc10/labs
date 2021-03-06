#include "config.h"
#include "pt_cornell_1_1.h"
#include "ir/IRremoteInt.h"
#include "ir/IRremote.h"
#include <stdlib.h>


static struct pt pt_uart, pt_receiver, pt_ir;
static unsigned char record_buf[35] = {0};
static unsigned char do_record = 0;
static unsigned char do_send = 0;
static unsigned char command, type;
static unsigned int value = 0;

static PT_THREAD ( fn_ir(struct pt* pt )) {
    decode_results results;
    int uart_i;
    unsigned int mask = 0;
    PT_BEGIN(pt);
    TIMER_ENABLE_PWM
    while( true ) {
        PT_YIELD_TIME_msec(100);
        if(decode(&results)) {
            if( do_record ) {
                printf("RECORD %d %u",results.decode_type, results.value);
                for( uart_i=0; uart_i<results.rawlen; uart_i++) {
                    printf(" %d", results.rawbuf[uart_i]);
                }
                printf("\n");
                do_record = 0;
            } else {
                if( do_send ) {
                    printf("SIGNAL type=%d value=%u ",
                        results.decode_type, results.value);
                    printf("COMMAND=%d TYPE=%d VALUE=%u\n",
                            command, type, value);
                    do_send = 0;
                }
            }
            resume();
        } else {
//            printf("rawLen = %d\n", irparams.rawlen);
        }
    }
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

static PT_THREAD (uartsend(struct pt *pt)) {
    static char character;
    PT_BEGIN(pt);
    num_send_chars = 0;
    while (PT_send_buffer[num_send_chars] != 0){
        PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
        //UARTSendDataByte(UART2, 0x42);  // this works
        //UARTSendDataByte(UART2, 'a');
        UARTSendDataByte(UART2, PT_send_buffer[num_send_chars]);
        num_send_chars++;
    }   
    
    PT_YIELD_TIME_msec(1000);
    // kill this output thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

/*
    // The fields are ordered to reduce memory over caused by struct-padding
	uint8_t       rcvstate;        // State Machine state
	uint8_t       recvpin;         // Pin connected to IR data from detector
	uint8_t       blinkpin;
	uint8_t       blinkflag;       // true -> enable blinking of pin on IR processing
	uint8_t       rawlen;          // counter of entries in rawbuf
	unsigned int  timer;           // State timer, counts 50uS ticks.
	unsigned int  rawbuf[RAWBUF];  // raw data
	uint8_t       overflow;        // Raw buffer overflow occurred
 */

unsigned int sony_raw[26] = {
    1000,
    // value = 0b0010011 = 0x13 = 19
    48,12,
    24,12,
    24,12,
    12,12,
    12,12,
    24,12,
    12,12,
    12,12,
    // address = 0b00001 = 0x01 = 1
    24,12,
    12,12,
    12,12,
    12,12,
    12
};

static PT_THREAD (uartreceiver(struct pt *pt))
{
    static char character;

    static int over_flow = 0;
    // mark the beginnning of the input thread
    PT_BEGIN(pt);
    
//    static unsigned char command, type;
//    static unsigned int value = 0;
//    
    num_char = 0;
    //memset(term_buffer, 0, max_chars);

//    irparams.rcvstate = STATE_STOP;
//    irparams.rawlen = 26;
//    memcpy(irparams.rawbuf, sony_raw, 26*sizeof(int));
//    
    decode_results results;
    
    while(num_char < max_chars) {
        // get the character
        // yield until there is a valid character so that other
        // threads can execute
        PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART2));
        // while(!UARTReceivedDataIsAvailable(UART2)){};
        command = UARTGetDataByte(UART2);
        if( command == '1' ) {
            // SEND
            do_send = 1;
            PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART2));
            type = UARTGetDataByte(UART2)-'0';
            value = 0;
            over_flow = 0;
            while( 1 ) {
                PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART2));
                // while(!UARTReceivedDataIsAvailable(UART2)){};
                character = UARTGetDataByte(UART2);
                over_flow ++;
                if ( over_flow >= 15 ) {
                    break;
                }
                if( character == '\r' ){
                    // END OF COMMAND
                    break;
                } else {
                    value *= 10;
                    value += (character-'0');
                }
            }
//            printf("COMMAND=%d TYPE=%d VALUE=%u\n",
//                            command, type, value);
            if( type == SONY) {
            } else if( type == SAMSUNG) {
                sendSAMSUNG(value, 32);
            } else {
            }
        } else if( command == '0' ) {
            // RECORD
            do_record = 1;
            PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART2));
            // while(!UARTReceivedDataIsAvailable(UART2)){};
            character = UARTGetDataByte(UART2);
            if( character == '\r' ) {
                // we are cool
            }
        } else {
            printf("COMMAND=%d UNKNOWN\n", character);
        }
        //if (character == backspace)
    } //end while(num_char < max_size)

    // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

// Code for uart
void main() { 
    ANSELA = 0; ANSELB = 0; 
    PPSInput (2, U2RX, RPA1); //Assign U2RX to pin RPA1 -- 
    // The TX pin must be one of the Group 4 output pins
    // RPA3, RPB0, RPB9, RPB10, RPB14 
    PPSOutput(4, RPB10, U2TX); //Assign U2TX to pin RPB10 -- 

    UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART2, pb_clock, BAUDRATE);
    UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
  
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 2000);//timer_limit);
    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag
    
    PT_setup();
    INTEnableSystemMultiVectoredInt();
    PT_INIT(&pt_ir);
    PT_INIT(&pt_receiver);
   
    enableIRIn();
//    enableIROut(38);
    
    OpenTimer3(T3_ON | T3_SOURCE_INT | T3_PS_1_1, 1052);
    OpenOC3(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE , 0, 0);
    PPSOutput(4, RPB14, OC3);
//    generate_period = 1052;
//    TIMER_ENABLE_PWM
//            
    sprintf(PT_send_buffer,"AT\r\n");

    while(1) {
        PT_SCHEDULE(fn_ir(&pt_ir));
        PT_SCHEDULE(uartreceiver(&pt_receiver));
    }
}
