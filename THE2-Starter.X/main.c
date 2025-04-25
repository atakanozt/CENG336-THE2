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

// Game states
#define STATE_PLAYING      0
#define STATE_SOFT_RESET   1
#define STATE_HARD_RESET   2

// Timer related definitions (in multiples of 5ms)
#define PRIZE_BLINK_PERIOD 500     // 500ms
#define GRAVITY_PERIOD     350      // 350ms
#define SCORE_PERIOD       1000     // 1000ms (1 second)
#define RESET_BLINK_PERIOD 400      // 400ms
#define RESET_TOTAL_TIME   2000     // 2000ms (2 seconds)

// Game related definitions
#define MAX_HIPPO_SIZE     5       // Maximum size before reset
#define INITIAL_SCORE      100     // Starting score for each round
#define SCORE_DECREMENT    10      // Points lost per second

// LED positions
#define PRIZE_POSITION     0       // Prize at RD0
#define MAX_POSITION       7       // RD7

// 7-segment display patterns for digits 0-9
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

// Struct definitions
typedef struct {
    uint8_t hippoSize;         // Size of the hippo (1-5)
    uint8_t hippoPosition;     // Position of hippo head
    uint8_t prizeVisible;      // Is the prize currently visible (blinking state)
    uint16_t currentScore;     // Score for the current round (0-100)
    uint16_t totalScore;       // Total accumulated score
    uint8_t gameState;         // Current game state
    uint32_t roundStartTime;   // Time when the current round started (in ms)
} GameState;

typedef struct {
    uint16_t gravityCounter;   // Counter for gravity tick (reset at 350ms)
    uint16_t blinkCounter;     // Counter for prize blink (reset at 500ms)
    uint16_t scoreCounter;     // Counter for score degradation (reset at 1000ms)
    uint16_t resetCounter;     // Counter for reset blinking (used during soft reset)
    uint16_t totalResetTime;   // Total time in reset state
    uint32_t totalTime;        // Total time since game started (in ms)
} Timers;

// ============================ //
//          GLOBALS             //
// ============================ //

GameState gameState;
Timers timers;
uint8_t currentDisplayIndex = 0;    // Current 7-segment display being updated (0-3)

// ============================ //
//          FUNCTIONS           //
// ============================ //

// Initialize ports and other settings
void initializeSystem() {
    // Set all ports as digital
    ADCON1 = 0x0F;
    
    // Configure PORTD as output for game display
    TRISD = 0x00;
    PORTD = 0x00;  // Clear all LEDs
    
    // Configure PORTH and PORTJ as outputs for 7-segment display
    TRISH = 0x00;
    TRISJ = 0x00;
    PORTH = 0x00;  // All displays off initially
    PORTJ = 0x00;  // All segments off initially
    
    // Configure PORTB0 as input for button
    TRISB = 0x01;  // RB0 as input, others as output
    
    // Configure INT0 for button press
    INTCON2bits.INTEDG0 = 1;  // Rising edge detection
    INTCONbits.INT0IE = 1;    // Enable INT0 interrupt
    INTCONbits.INT0IF = 0;    // Clear INT0 flag
    
    // Configure TIMER0 for 5ms interrupt
    T0CON = 0x08;               // 16-bit, internal clock, prescaler off initially
    TMR0H = 0x9E;               // Initialize for 5ms @ 40MHzx
    TMR0L = 0x58;               // (65535 - 50000) = 0x3CAF
    T0CONbits.PSA = 0;          // enable prescaler
    T0CONbits.T0PS = 0b000;     // 1:8 prescaler
    INTCONbits.TMR0IE = 1;      // Enable Timer0 interrupt
    INTCONbits.TMR0IF = 0;      // Clear Timer0 flag
    
    // Configure high-priority interrupts
    RCONbits.IPEN = 1;          // Enable priority levels
    INTCONbits.GIEH = 1;        // Enable high-priority interrupts
}

// Initialize game state
void initializeGame() {
    // Initialize game state
    gameState.hippoSize = 1;
    gameState.hippoPosition = MAX_POSITION;
    gameState.prizeVisible = 1;
    gameState.currentScore = INITIAL_SCORE;
    gameState.totalScore = 0;
    gameState.gameState = STATE_PLAYING;
    gameState.roundStartTime = 0;
    
    // Initialize timers
    timers.gravityCounter = 0;
    timers.blinkCounter = 0;
    timers.scoreCounter = 0;
    timers.resetCounter = 0;
    timers.totalResetTime = 0;
    timers.totalTime = 0;
    
    // Clear display
    PORTD = 0x00;
}

// Update the 7-segment displays with the score
void updateAllDisplays() {
    uint16_t score = gameState.totalScore;
    
    // Update all four displays in sequence
    for (uint8_t i = 0; i < 4; i++) {
        // Turn off all displays
        PORTH = 0x00;
        
        // Calculate digit to display
        uint8_t digit = 0;
        switch (i) {
            case 0: // Rightmost digit
                digit = score % 10;
                break;
            case 1:
                digit = (score / 10) % 10;
                break;
            case 2:
                digit = (score / 100) % 10;
                break;
            case 3: // Leftmost digit
                digit = (score / 1000) % 10;
                break;
        }
        
        // Set segments for current digit
        PORTJ = SEVEN_SEG_DIGITS[digit];
        
        // Enable the current display
        PORTH = (1 << i);
        
        // Small delay to allow display to stabilize
        __delay_us(500);
    }
}

// Update the game display on PORTD
void updateGameDisplay() {
    // Start with all LEDs turned off
    uint8_t display = 0x00;
    
    // CASE 1: During soft reset (after eating prize) - special blinking pattern
    if (gameState.gameState == STATE_SOFT_RESET) {
        // Calculate which blink cycle we're in (each cycle is 400ms)
        uint8_t blinkCycle = timers.totalResetTime / RESET_BLINK_PERIOD;
        
        // For even cycles (0, 2, 4) turn ALL LEDs on
        // For odd cycles (1, 3) turn ALL LEDs off
        if (blinkCycle % 2 == 0) {
            display = 0xFF;  // 11111111 - All LEDs on
        } else {
            display = 0x00;  // 00000000 - All LEDs off
        }
    }
    // CASE 2: Normal gameplay
    else {
        // Step 1: Display the prize at RD0 (the top-most LED) if it's visible
        if (gameState.prizeVisible) {
            // Set the bit at position 0 (RD0) to 1
            display = 0x01;  // Same as (1 << 0) or 0b00000001
        }
        
        // Step 2: Display the hippo
        // The hippo is made up of 1 to 5 consecutive LEDs
        // The head is at hippoPosition, and the body extends upward
        for (uint8_t i = 0; i < gameState.hippoSize; i++) {
            // Calculate position for each segment:
            // - First segment (i=0) is at hippoPosition (the head)
            // - Next segments (i=1,2,3...) extend upward (lower RD values)
            uint8_t pos = gameState.hippoPosition - i;
            
            // Only show the segment if it's within valid LED range
            if (pos <= MAX_POSITION) {  // MAX_POSITION is 7 (RD7)
                // Turn on the LED at position 'pos'
                
                // Create a value with only bit 'pos' set to 1
                uint8_t bitMask = 0;
                
                // Set the appropriate bit based on position
                switch(pos) {
                    case 0: bitMask = 0b00000001; break; // RD0 (Prize position)
                    case 1: bitMask = 0b00000010; break; // RD1
                    case 2: bitMask = 0b00000100; break; // RD2
                    case 3: bitMask = 0b00001000; break; // RD3
                    case 4: bitMask = 0b00010000; break; // RD4
                    case 5: bitMask = 0b00100000; break; // RD5
                    case 6: bitMask = 0b01000000; break; // RD6
                    case 7: bitMask = 0b10000000; break; // RD7 (Bottom position)
                }
                
                // Combine the new bit with existing display
                // This is equivalent to display |= bitMask
                display = display | bitMask;
            }
        }
    }
    
    // Finally, update the physical LED display by writing to PORTD
    PORTD = display;
}

// Move hippo up by one position
void moveHippoUp() {
    if (gameState.hippoPosition > 0) {
        gameState.hippoPosition--;
    }
}

// Move hippo down by one position (gravity)
void moveHippoDown() {
    // Only move down if hippo's head isn't at the bottom
    if (gameState.hippoPosition < MAX_POSITION) {
        gameState.hippoPosition++;
    }
}

// Check if hippo has caught the prize
uint8_t checkPrizeCaught() {
    // Check if any part of the hippo is at the prize position
    for (uint8_t i = 0; i < gameState.hippoSize; i++) {
        uint8_t pos = gameState.hippoPosition - i;
        if (pos == PRIZE_POSITION) {
            return 1;
        }
    }
    return 0;
}

// Soft reset after prize is caught
void softReset() {
    // Update total score with current round score
    gameState.totalScore += gameState.currentScore;
    
    // Change game state to soft reset
    gameState.gameState = STATE_SOFT_RESET;
    
    // Reset timers for blinking
    timers.resetCounter = 0;
    timers.totalResetTime = 0;
}

// Hard reset when hippo size reaches max
void hardReset() {
    // Reset hippo size to 1
    gameState.hippoSize = 1;
    
    // Reset hippo position to bottom
    gameState.hippoPosition = MAX_POSITION;
    
    // Reset current score
    gameState.currentScore = INITIAL_SCORE;
    
    // Change game state back to playing
    gameState.gameState = STATE_PLAYING;
    
    // Reset round start time
    gameState.roundStartTime = timers.totalTime;
}

// Complete the soft reset and prepare for next round
void completeSoftReset() {
    // Increase hippo size
    gameState.hippoSize++;
    
    // Check if hippo size reached max
    if (gameState.hippoSize > MAX_HIPPO_SIZE) {
        hardReset();
    } else {
        // Make sure hippo is positioned at bottom
        gameState.hippoPosition = MAX_POSITION;
        
        // Reset current score for next round
        gameState.currentScore = INITIAL_SCORE;
        
        // Return to playing state
        gameState.gameState = STATE_PLAYING;
        
        // Reset prize visibility
        gameState.prizeVisible = 1;
        
        // Reset round start time
        gameState.roundStartTime = timers.totalTime;
    }
}

// Process timer tick (5ms)
void processTimerTick() {
    // Update total time
    timers.totalTime += 5;
    
    // Only process game logic if we're in playing state
    if (gameState.gameState == STATE_PLAYING) {
        // Handle prize blinking
        timers.blinkCounter += 5;
        if (timers.blinkCounter >= PRIZE_BLINK_PERIOD) {
            gameState.prizeVisible = !gameState.prizeVisible;
            timers.blinkCounter = 0;
        }
        
        // Handle gravity
        timers.gravityCounter += 5;
        if (timers.gravityCounter >= GRAVITY_PERIOD) {
            moveHippoDown();
            timers.gravityCounter = 0;
        }
        
        // Handle score degradation
        timers.scoreCounter += 5;
        if (timers.scoreCounter >= SCORE_PERIOD) {
            if (gameState.currentScore >= SCORE_DECREMENT) {
                gameState.currentScore -= SCORE_DECREMENT;
            } else {
                gameState.currentScore = 0;
            }
            timers.scoreCounter = 0;
        }
        
        // Check if prize caught
        if (checkPrizeCaught()) {
            softReset();
        }
    } 
    // Handle soft reset state
    else if (gameState.gameState == STATE_SOFT_RESET) {
        timers.resetCounter += 5;
        timers.totalResetTime += 5;
        
        // End of reset period
        if (timers.totalResetTime >= RESET_TOTAL_TIME) {
            completeSoftReset();
        }
    }
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //
void __interrupt(high_priority) HandleInterrupt()
{
    // INT0 interrupt (button press)
    if (INTCONbits.INT0IF) {
        // Only move hippo up if in playing state
        if (gameState.gameState == STATE_PLAYING) {
            moveHippoUp();
        }
        
        // Clear the interrupt flag
        INTCONbits.INT0IF = 0;
    }
    
    // Timer0 interrupt (5ms)
    if (INTCONbits.TMR0IF) {
        // Reset Timer0
        TMR0H = 0x9E;               // Initialize for 5ms @ 40MHzx
        TMR0L = 0x58;
        
        // Process the timer tick
        processTimerTick();
        
        // Clear the interrupt flag
        INTCONbits.TMR0IF = 0;
    }
}

// ============================ //
//            MAIN              //
// ============================ //
void main()
{
    // Initialize the system
    initializeSystem();
    
    // Initialize the game
    initializeGame();
    
    // Wait 1 second before starting
    for (uint16_t i = 0; i < 200; i++) {
        // Update display during wait
        updateAllDisplays();
        __delay_ms(5);
    }
    
    // Start Timer0
    T0CONbits.TMR0ON = 1;
    
    // Main game loop
    while (1) {
        // Update the game display
        updateGameDisplay();
        
        // Update all 7-segment displays
        updateAllDisplays();
    }
}