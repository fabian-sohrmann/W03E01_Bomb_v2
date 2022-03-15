/*
 * File:   main.c
 * Author: Fabian Sohrmann
 * Email:  mfsohr@utu.fi
 * 
 * This program simulates diffusing a bomb. It is a different implementation
 * of the exercise W2E01, this time using the RTC module.
 * 
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * THE 7-SEGMENT DISPLAY IS SWITCHED DIFFERENTLY THAN IN EXERCISE W2E01. THE
 * SAME PORT (C) IS USED BUT THE PIN CONFIGURATION IS DIFFERENT. THIS IS DUE
 * TO THE GIVEN SCHEMATIC, WHICH IS A LITTLE BIT DIFFERENT. I CHANGED THE 
 * BIT PATTERNS FOR THE SEGMENTS ACCORDINGLY, SO ON MY TEST RUNS THE NUMBERS
 * WHERE DISPLAYED CORRECTLY!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Created on 10. marraskuuta 2021, 18:41
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


volatile int g_running = 1;
volatile int g_clockticks = 0;

/*
* This function is copied and modified from technical brief TB3213 and
 * recopied and remodified from the provided example in the materials.
*/
void rtc_init(void)
{
    uint8_t temp;
    // Disable oscillator
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_ENABLE_bm;
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    // Wait for the clock to be released (0 = unstable, unused)
    while (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm);

    // Select external crystal (SEL = 0)
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_SEL_bm;
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);

    // Enable oscillator
    temp = CLKCTRL.XOSC32KCTRLA;
    temp |= CLKCTRL_ENABLE_bm;
    ccp_write_io((void*)&CLKCTRL.XOSC32KCTRLA, temp);
    // Wait for the clock to stabilize
    while (RTC.STATUS > 0);
    
    // Configure RTC module
    // Select 32.768 kHz external oscillator
    RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;
    // Enable Periodic Interrupt
    RTC.PITINTCTRL = RTC_PI_bm;
    // Set period to 16384 cycles (1/2 second) and enable PIT function
    
    // MODIFIED TO 4096 CYCLES, INTERRUPTS 8 TIMES / SECOND
    RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm;     
}

int main(void) 
{
    rtc_init(); 
    sei(); // Enable interrupts
    
    int countdown_time = 9;
    
    // Bit patterns for displaying numbers 0...9
    uint8_t number[] =
    {
        0b11101111, 0b10000011, 0b11011101, 0b11010111,
        0b10110011, 0b01110111, 0b01111111, 0b11000011,  
        0b11111111, 0b11110111
    };
    
    PORTC.DIRSET = 0xFF; // Setting all pins in port C to output
    PORTA.DIRCLR = PIN4_bm; // PIN4 to input (not necessay, same as default)
    // WRONG PORTA.PIN4CTRL |= PORT_PULLUPEN_bm; // Enable PA4 pull-up resistor
    PORTA.PIN4CTRL = PORT_ISC_RISING_gc | PORT_PULLUPEN_bm; 
    
    PORTF.DIRSET = PIN5_bm; // PIN5 to output
    //WRONG PORTF.PIN5CTRL |= PORT_PULLUPEN_bm; // Enable PF5 pull-up resistor
    PORTF.OUTSET = PIN5_bm; // PIN5 to high turns display on
    
    while (1) 
    {
        while(g_running)
        {
            // 8 interrupts equals 1 second, so interrupts 0-7 = 1 second, 
            // 8-15 = 2s... 72-79 = 9s.
            // Loop uses interrupt count, when interrupts are <=72 
            // interrupt count is enough to countdown to zero.
            if(g_clockticks<=72)
            {
                if((g_clockticks%8)==0) // Run every 1 sec (every 8 interrupts)
                {
                PORTC.OUTCLR = 0xFF; // Clear pins
                // Draw numbers according to number bit patterns in number[]
                // and calculate right index for each number
                PORTC.OUTSET = number[countdown_time-(g_clockticks/8)];
                }
            }
            else
            {
                PORTF.OUTTGL = PIN5_bm; // Toggle high/low -> display on/off
            }
            sleep_mode();
        }
    }
}

ISR(PORTA_PORT_vect)
{
    PORTA.INTFLAGS = 0xFF; // Clear interrupt flag
    g_running = 0; // Stop loop
}

ISR(RTC_PIT_vect)
{
    RTC.PITINTFLAGS = RTC_PI_bm; // Clear interrupt flag
    g_clockticks++; // Increase for each interrupt
}