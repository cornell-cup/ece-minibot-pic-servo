/*
 * File:   main.c
 * Author: Syed Tahmid Mahbub
 *
 * Created on April 27, 2016
 */


#include <xc.h>
#include <stdint.h>


// Generated with configuration bits tool in MPLAB X

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG
#pragma config FOSC = INTRCIO  //  (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select bit (MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Detect (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)


#define SERVO_MIN       1200 // 600us
#define SERVO_REST      3000 // 1.5 ms
#define SERVO_MAX       4800 // 2.4 ms
//#define SERVO_MIN       2000 // 1ms
//#define SERVO_REST      3000 // 1.5 ms
//#define SERVO_MAX       300 // 2 ms
//#define NUMSTEPS        200
#define PERIOD          40000-1 // 20 ms
#define STEP            18

volatile uint8_t servo = 0;
volatile uint16_t ccp, wait;
volatile uint8_t servoh, servol, waith, waitl;

void main(void) {    
    OSCCON = 0x71;  // 8 MHz
    while (OSCCONbits.HTS == 0); // wait for clock to stabilize
    
    CM1CON0 = 7;    // disable comparator 1
    CM2CON0 = 7;    // disable comparator 2
    ANSEL = 0;      // disable adc
    ANSELH = 0;
    
    // SERVO OUTPUT = RC5
    TRISCbits.TRISC5 = 0;
   
    INTCONbits.GIE = 1; // enable global interrupts
    INTCONbits.PEIE = 1; // enable peripheral interrupt
    PIR1bits.CCP1IF = 0;
    PIE1bits.CCP1IE = 1;
    
    servo = 0;
    
    ccp = (uint16_t) SERVO_MIN + servo*STEP;
    if (ccp > SERVO_MAX) ccp = SERVO_MAX;
    servoh = ccp >> 8;
    servol = ccp;
    wait = (uint16_t) PERIOD - ccp;
    waith = wait >> 8;
    waitl = wait;
        
    CCP1CON = 0x09; // Compare - clear output on match
    CCPR1H = servoh; CCPR1L = servol;
    T1CON = 1;
    
    TRISCbits.TRISC4 = 0;
    
    // Mode 1: CPOL/CKP=0, CKE/NCPHA=0
    SSPSTATbits.SMP = 0; // must be zero for slave
    SSPSTATbits.CKE = 0;
    SSPCON = 0x25; // SPI enabled as slaved, no SS
    TRISCbits.TRISC7 = 0; // SDO
    TRISBbits.TRISB6 = 1; // SCK
    TRISBbits.TRISB4 = 1; // SDI
    TRISAbits.TRISA0 = 0; // LED
    PORTAbits.RA0 = 0;
    while (1){
        uint8_t spi_data;
        if (SSPSTATbits.BF){    // SPI data received
            spi_data = SSPBUF;
            if (spi_data >= 40 && spi_data <= 240){
                servo = spi_data - 40;
            }
        }
    }
}

void interrupt ccpint(void){ // only interrupt source is ccp1
    TMR1H = 0; TMR1L = 0;   // reset timer to maintain count accuracy
    if (CCP1CON == 9){ // now wait
        CCP1CON = 8;
        CCPR1H = waith;
        CCPR1L = waitl;
        PORTCbits.RC4 = 0;
        
        ccp = (uint16_t) SERVO_MIN + servo*STEP;
        if (ccp > SERVO_MAX) ccp = SERVO_MAX;
        servoh = ccp >> 8;
        servol = ccp;
        wait = (uint16_t) PERIOD - ccp;
        waith = wait >> 8;
        waitl = wait;
    }
    else{
        CCP1CON = 9; // now drive
        CCPR1H = servoh;
        CCPR1L = servol;
        PORTCbits.RC4 = 1;
       
    }

    PIR1bits.CCP1IF = 0;    // clear ccp1 interrupt flag to resume main
}

