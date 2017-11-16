#include "config.h"
#include "pt_cornell_1_1.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>

static struct pt pt_uart,pt_receiver;

static PT_THREAD (uartsend(struct pt *pt))
{
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

static PT_THREAD (uartreceiver(struct pt *pt))
{
    static char character;
    // mark the beginnning of the input thread
    PT_BEGIN(pt);

    num_char = 0;
    //memset(term_buffer, 0, max_chars);

    while(num_char < max_chars)
    {
        // get the character
        // yield until there is a valid character so that other
        // threads can execute
        PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART2));
       // while(!UARTReceivedDataIsAvailable(UART2)){};
        character = UARTGetDataByte(UART2);
        PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
        UARTSendDataByte(UART2, character);

        // unomment to check backspace character!!!
        //printf("--%x--",character );

        // end line
        if(character == '\r'){
            PT_term_buffer[num_char] = 0; // zero terminate the string
            //crlf; // send a new line
            PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
            UARTSendDataByte(UART2, '\n');
            break;
        }
        // backspace
        else if (character == backspace){
            PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
            UARTSendDataByte(UART2, ' ');
            PT_YIELD_UNTIL(pt, UARTTransmitterIsReady(UART2));
            UARTSendDataByte(UART2, backspace);
            num_char--;
            // check for buffer underflow
            if (num_char<0) {num_char = 0 ;}
        }
        else  {PT_term_buffer[num_char++] = character ;}
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
  
   PT_setup();
   INTEnableSystemMultiVectoredInt();
   PT_INIT(&pt_uart);
   PT_INIT(&pt_receiver);
   
  sprintf(PT_send_buffer,"AT\r\n");
 
  while(1) {
      //PT_SCHEDULE(uartsend(&pt_uart));
      PT_SCHEDULE(uartreceiver(&pt_receiver));
  }
  
}
