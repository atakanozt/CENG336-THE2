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

// You can write struct definitions here...

// ============================ //
//          GLOBALS             //
// ============================ //
#define BLINK_PERIOD 500
#define GRAVITY_PERIOD 350
#define SCORE_DECR_PERIOD 10
#define SOFT_RESET_PERIOD 400
#define INITIAL_ROUND_SCORE 100
#define SOFT_RESET_TOTAL_PERIOD 2000



#define NOT_STARTED 0
#define GAME_PLAYING 1
#define SOFT_RESET 2
#define HARD_RESET 3

#define START_POSITION 7
#define MAX_POSITION 7


const unsigned char DIGITS[10] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111,
};


typedef struct {
    uint8_t hippoSize;
    uint8_t hippoPosition;
    uint8_t gameState;
    uint8_t prizeVisible;
    uint16_t roundScore;
    uint16_t totalScore;
} GAMESTATE;
GAMESTATE GameState;


typedef struct {
    uint16_t blinkCounter;
    uint16_t gravityCounter;
    uint16_t scoreCounter;
    uint16_t resetCounter;
    uint16_t resetTime;
    uint32_t totalTime;
} TIMER;
TIMER Timers;

// You can write globals definitions here...

// ============================ //
//          FUNCTIONS           //
// ============================ //
void initializeSystem()
{
    TRISB = 0x01;
    
    TRISD = 0x00;
    TRISH = 0x00;
    TRISJ = 0x00;
    
    LATD = 0x00;
    LATH = 0x00;
    LATJ = 0x00;
    
    INTCON = 0x00;
    INTCONbits.TMR0IE = 1;
    INTCONbits.INT0IE = 1;
    T0CON = 0x08;
    TMR0 = 0x9E58;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 0b000;
}


void initializeGame(){
    GameState.gameState = NOT_STARTED;
    GameState.hippoPosition = START_POSITION;
    GameState.hippoSize = 1;
    GameState.prizeVisible = 1;
    GameState.roundScore = INITIAL_ROUND_SCORE;
    GameState.totalScore = 0;
    
    
    Timers.blinkCounter = 0;
    Timers.gravityCounter = 0 ;
    Timers.scoreCounter = 0;
    Timers.resetCounter = 0;
    Timers.resetTime = 0;
    Timers.totalTime = 0;
}

void moveHippoDown(){
    if(GameState.hippoPosition + GameState.hippoSize - 1 >= MAX_POSITION ){
       ; 
    }
    else{
        GameState.hippoPosition += 1;
    }
}

void decrement_round_score()
{
    if(GameState.roundScore <= 0)
    {
        ;
    }
    else{
        GameState.roundScore -= SCORE_DECR_PERIOD;
    }
}

void hardReset(){
    GameState.hippoSize = 1;
    GameState.hippoPosition = MAX_POSITION - GameState.hippoSize + 1;
    GameState.gameState = GAME_PLAYING;
    GameState.prizeVisible = 1;
    GameState.roundScore = INITIAL_ROUND_SCORE;
}

void complete_soft_reset(){
    if(GameState.hippoSize >= 5){
        hardReset();
    }
    else{
        GameState.hippoSize += 1;
        GameState.hippoPosition = MAX_POSITION - GameState.hippoSize + 1;
        GameState.gameState = GAME_PLAYING;
        GameState.prizeVisible = 1;
        GameState.roundScore = INITIAL_ROUND_SCORE;
    }   
}

void process_timer(){
    Timers.totalTime += 5;
    
    
    if(GameState.gameState == GAME_PLAYING)
    {
        Timers.blinkCounter += 5;
        if(Timers.blinkCounter >= BLINK_PERIOD)
        {
            GameState.prizeVisible = !GameState.prizeVisible;
            Timers.blinkCounter = 0;
        }
        
        Timers.gravityCounter += 5;
        if(Timers.gravityCounter >= GRAVITY_PERIOD)
        {
            moveHippoDown();
            Timers.gravityCounter = 0;
        }
        
        Timers.scoreCounter += 5;
        if(Timers.scoreCounter >= 1000)
        {
            decrement_round_score();
            Timers.scoreCounter = 0;
        }
    }
    
    if(GameState.gameState == SOFT_RESET){
        Timers.resetCounter += 5;
        if(Timers.resetCounter >= SOFT_RESET_TOTAL_PERIOD)
        {
            complete_soft_reset();
            Timers.resetCounter = 0;
        }
    }
    
}

uint8_t checkPrizeCaught()
{
    if(GameState.hippoPosition == 0)
        return 1;
    
    return 0;
}

void process_rb0(){
    GameState.hippoPosition -= 1;
    if(checkPrizeCaught() == 1)
    {
        GameState.totalScore += GameState.roundScore;
        GameState.gameState = SOFT_RESET;
    }
}



void updateGameDisplay(){
    uint8_t portd_value = 0x00;
    
    if(GameState.gameState == GAME_PLAYING)
    {        
        if(GameState.prizeVisible == 1)
            portd_value |= 0x01; 
            
        
        for(uint8_t i = 0; i < GameState.hippoSize; i++)
        {
            uint8_t pos = GameState.hippoPosition + i;
            switch(pos)
            {
                case 1: 
                    portd_value |= 0b00000010;
                    break;
                case 2: 
                    portd_value |= 0b00000100;
                    break;
                case 3: 
                    portd_value |= 0b00001000;
                    break;
                case 4: 
                    portd_value |= 0b00010000;
                    break;
                case 5: 
                    portd_value |= 0b00100000;
                    break;
                case 6: 
                    portd_value |= 0b01000000;
                    break;
                case 7: 
                    portd_value |= 0b10000000;
                    break;  
            }
        }
    }
    if(GameState.gameState == SOFT_RESET)
    {
        uint8_t blinkCycle = Timers.resetCounter / SOFT_RESET_PERIOD;
        if(blinkCycle % 2 == 0)
        {
            portd_value = 0xFF;
        }
        else{
            portd_value = 0x00;
        }
    }
    LATD = portd_value;   
}


void updateAllDisplay(){
    uint16_t score = GameState.totalScore;
    
    for(uint8_t i=0; i < 4; i++){
        LATH = 0x00;
        
        uint8_t digit = 0;
        uint8_t lath_val;
        if(i == 3)
        {
            digit = score % 10;
            lath_val = 0b00001000;
        }
        if(i == 2)
        {
            digit = (score/10) % 10;
            lath_val = 0b00000100;
        }
        if(i == 1)
        {
            digit = (score/100) % 10;
            lath_val = 0b00000010;
        }
        if(i == 0)
        {
            digit = (score/1000) % 10;
            lath_val = 0b00000001;
        }
        LATJ = DIGITS[digit];
        LATH = lath_val;
        
        __delay_us(500);
    }
}

// You can write function definitions here...

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
__interrupt(high_priority)
void HandleInterrupt()
{
    // ISR ...
    if(INTCONbits.INT0IF)
    {
        process_rb0();
        INTCONbits.INT0IF = 0;
    }
    
    if(INTCONbits.TMR0IF)
    {
        TMR0H = 0x9E;
        TMR0L = 0x58;
        process_timer();
        INTCONbits.TMR0IF = 0;
    }
}

// ============================ //
//            MAIN              //
// ============================ //
void main()
{
    // Main ...
    initializeSystem();
    
    initializeGame();
    
    for(uint8_t i =0; i<200; i++){
        updateAllDisplay();
        __delay_ms(5);
    }
    INTCONbits.GIE =1;
    T0CONbits.TMR0ON = 1;
    
    GameState.gameState = GAME_PLAYING;
    while(1){
        updateGameDisplay();
        updateAllDisplay();
    }
}
