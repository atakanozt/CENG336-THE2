// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL      // Oscillator Selection bits (HS oscillator,
                                // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit
                                // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit
                                // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out
                                // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT = OFF        // Watchdog Timer Enable bit
                                // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit
                                // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled;
                                // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply
                                // ICSP disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit
                                // (Instruction set extension and Indexed
                                // Addressing mode disabled (Legacy mode))
#pragma config DEBUG = OFF      // Disable In-Circuit Debugger

// Timer Related Definitions
#define KHZ 1000UL
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ (40UL * MHZ)
// ============================ //
//             End              //
// ============================ //
#include <xc.h>
#include <stdint.h>

// ============================ //
//        DEFINITIONS           //
// ============================ //

#define STATE_PLAYING      0
#define STATE_SOFT_RESET   1

#define PRIZE_BLINK_PERIOD 500     
#define GRAVITY_PERIOD     350
#define SCORE_PERIOD       1000     
#define RESET_BLINK_PERIOD 400      
#define RESET_TOTAL_TIME   2000     

// Game related definitions
#define MAX_HIPPO_SIZE     5      
#define INITIAL_SCORE      100     
#define SCORE_DECREMENT    10      

// LED positions
#define PRIZE_POSITION     0       
#define MAX_POSITION       7       

const uint8_t SEVEN_SEG_DIGITS[10] = {
    0b00111111, // 0: A, B, C, D, E, F
    0b00000110, // 1: B, C
    0b01011011, // 2: A, B, D, E, G
    0b01001111, // 3: A, B, C, D, G
    0b01100110, // 4: B, C, F, G
    0b01101101, // 5: A, C, D, F, G
    0b01111101, // 6: A, C, D, E, F, G
    0b00000111, // 7: A, B, C
    0b01111111, // 8: A, B, C, D, E, F, G
    0b01101111  // 9: A, B, C, D, F, G
};


// ============================ //
//          GLOBALS             //
// ============================ //

uint8_t currentDisplayIndex = 0; 

uint8_t hippoSize;         
uint8_t hippoPosition;    
uint8_t prizeVisible;      
uint16_t currentScore;     
uint16_t totalScore;       
uint8_t gameState;       
uint32_t roundStartTime;  


uint16_t gravityCounter;   
uint16_t blinkCounter;     
uint16_t scoreCounter;     
uint16_t resetCounter;     
uint16_t totalResetTime;   
uint32_t totalTime;        


// ============================ //
//          FUNCTIONS           //
// ============================ //

void initializeSystem() {
    ADCON1 = 0x0F;
    
    TRISD = 0x00;
    LATD = 0x00; 
    
    TRISH = 0x00;
    TRISJ = 0x00;
    LATH = 0x00;  
    LATJ = 0x00; 
    
    TRISB = 0x01; 
    
    INTCONbits.INT0IE = 1;    // enable int0
    INTCONbits.INT0IF = 0;    // clear int0
    

    T0CON = 0x08;            
    TMR0H = 0x9E;               
    TMR0L = 0x58;            
    T0CONbits.PSA = 0;          // enable prescaler
    T0CONbits.T0PS = 0b000;     // presca=2
    INTCONbits.TMR0IE = 1;      // enable tmr0
    INTCONbits.TMR0IF = 0;      // clear tmr0 interrupt
    
    RCONbits.IPEN = 1;          // Enable priority 
    INTCONbits.GIEH = 1;        // Enable high-priority interrupts
}

void initializeGame()
{
    
    hippoSize = 1;
    hippoPosition = MAX_POSITION;
    prizeVisible = 1;
    currentScore = INITIAL_SCORE;
    totalScore = 0;
    gameState = STATE_PLAYING;
    roundStartTime = 0;
    
    gravityCounter = 0;
    blinkCounter = 0;
    scoreCounter = 0;
    resetCounter = 0;
    totalResetTime = 0;
    totalTime = 0;
    
    LATD = 0x00;
}

void updateAllDisplays()
{
    uint16_t score = totalScore;
    
    for (uint8_t i = 0; i < 4; i++) {
        LATH = 0x00;
        
        uint8_t digit = 0;
        uint8_t lath_value = 0;
        switch (i) {
            case 3:
                digit = score % 10;
                lath_value = 0b00001000;
                break;
            case 2:
                digit = (score / 10) % 10;
                lath_value = 0b00000100;
                break;
            case 1:
                digit = (score / 100) % 10;
                lath_value = 0b00000010;
                break;
            case 0: 
                digit = (score / 1000) % 10;
                lath_value = 0b00000001;
                break;
        }
        
        LATJ = SEVEN_SEG_DIGITS[digit];
        LATH = lath_value;
        
        __delay_us(500);
    }
}

void updateGameDisplay() {
    uint8_t display = 0x00;
    
    if (gameState == STATE_SOFT_RESET) {
        uint8_t blinkCycle = totalResetTime / RESET_BLINK_PERIOD;
        
        if (blinkCycle % 2 == 0) {
            display = 0xFF;  
        } else {
            display = 0x00; 
        }
    }
    else {
        if (prizeVisible) {
            display = 0b00000001;
        }
        
        for (uint8_t i = 0; i < hippoSize; i++) {
            uint8_t pos = hippoPosition - i;
            if (pos <= MAX_POSITION) 
            {  
                uint8_t bitMask = 0;
                switch(pos) {
                    case 0: bitMask = 0b00000001; break;
                    case 1: bitMask = 0b00000010; break; 
                    case 2: bitMask = 0b00000100; break; 
                    case 3: bitMask = 0b00001000; break; 
                    case 4: bitMask = 0b00010000; break;
                    case 5: bitMask = 0b00100000; break;
                    case 6: bitMask = 0b01000000; break; 
                    case 7: bitMask = 0b10000000; break; 
                }
                display = display | bitMask;
            }
        }
    }
    
    LATD = display;
}

void moveHippoUp() {
    if (hippoPosition > 0) {
        hippoPosition--;
    }
}

void moveHippoDown() {
    if (hippoPosition < MAX_POSITION) {
        hippoPosition++;
    }
}

uint8_t checkPrizeCaught() {
    for (uint8_t i = 0; i < hippoSize; i++) {
        uint8_t pos = hippoPosition - i;
        if (pos == PRIZE_POSITION) {
            return 1;
        }
    }
    return 0;
}

void softReset() {
    if (totalScore <= 9999 - currentScore) {
        totalScore += currentScore;
    }
    else {
        totalScore = 9999;
    }
    gameState = STATE_SOFT_RESET;
    
    resetCounter = 0;
    totalResetTime = 0;
    blinkCounter = 0;
}

void completeSoftReset() {
    hippoSize++;
    
    if (hippoSize > MAX_HIPPO_SIZE) { // hard reset
        hippoSize = 1;
        hippoPosition = MAX_POSITION;
        currentScore = INITIAL_SCORE;
        gameState = STATE_PLAYING;
        prizeVisible = 1;
        roundStartTime = totalTime;
    } else {

        hippoPosition = MAX_POSITION;
        currentScore = INITIAL_SCORE;
        gameState = STATE_PLAYING;
        prizeVisible = 1;
        roundStartTime = totalTime;
    }
}

void processTimerTick() {
    totalTime += 5;
    
    if (gameState == STATE_PLAYING) {
        blinkCounter += 5;
        if (blinkCounter >= PRIZE_BLINK_PERIOD) {
            prizeVisible = !prizeVisible;
            blinkCounter = 0;
        }
        
        gravityCounter += 5;
        if (gravityCounter >= GRAVITY_PERIOD) {
            moveHippoDown();
            gravityCounter = 0;
        }
        
        scoreCounter += 5;
        if (scoreCounter >= SCORE_PERIOD) {
            if (currentScore >= SCORE_DECREMENT) {
                currentScore -= SCORE_DECREMENT;
            } else {
                currentScore = 0;
            }
            scoreCounter = 0;
        }
        
        if (checkPrizeCaught()) {
            softReset();
        }
    } 
    else if (gameState == STATE_SOFT_RESET) {
        resetCounter += 5;
        totalResetTime += 5;
        
        if (totalResetTime >= RESET_TOTAL_TIME) {
            completeSoftReset();
        }
    }
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
void __interrupt(high_priority) HandleInterrupt()
{
    if (INTCONbits.INT0IF) {
        if (gameState == STATE_PLAYING) {
            moveHippoUp();
        }
        
        INTCONbits.INT0IF = 0;
    }
    
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x9E;            
        TMR0L = 0x58;
        
        processTimerTick();
        
        INTCONbits.TMR0IF = 0;
    }
}

// ============================ //
//            MAIN              //
// ============================ //
void main()
{
    initializeSystem();
    initializeGame();
    
    for (uint16_t i = 0; i < 200; i++) {
        __delay_ms(5);
    }
    
    T0CONbits.TMR0ON = 1;
    
    while (1) {
        updateGameDisplay();
        
        updateAllDisplays();
    }
}