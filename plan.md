# CENG336 THE2 - Hungry Hippo Game Implementation Plan

## Project Overview

This project involves implementing a variation of the "Hungry, Hungry Hippos" game on a PIC18F8722 microprocessor. The game displays on PORTD LEDs, where:

- A blinking prize appears at the top (RD0)
- A hippo starts at the bottom (RD7)
- The player must navigate the hippo to the prize while fighting against gravity
- As the hippo eats prizes, it grows in size
- Score is displayed on 7-segment displays

## Hardware Requirements

- PIC18F8722 microcontroller
- PORTD LEDs for game display
- PORTB0 for user input
- PORTH and PORTJ for 7-segment display output
- Timer0 for game timing

## Software Configuration

1. Configure the PIC18F8722 with a 40 MHz clock
2. Set up required interrupts:
   - INT0 for button press (PORTB0)
   - TIMER0 for game timing (prize blinking, gravity effects, score degradation)
3. Configure I/O ports:
   - PORTB0 as input
   - PORTD as output for game display
   - PORTH and PORTJ as outputs for 7-segment display

## Implementation Steps

### 1. Basic Setup and Initialization

- Configure oscillator and other required settings
- Initialize I/O ports and interrupts
- Clear all LEDs
- Initialize 7-segment displays to show zero
- Wait one second before starting the game
- Enable interrupts and start the game loop

### 2. TIMER0 Implementation

- Configure TIMER0 to trigger an interrupt every 5ms ±100μs
- Use TIMER0 for:
  - Blinking the prize every 500ms
  - Applying gravity to pull the hippo down every 350ms
  - Degrading score by 10 points every second (maximum score starts at 100)

### 3. Interrupt Handlers

- Implement INT0 interrupt handler:
  - Process button press/release to move hippo up by one position
- Implement TIMER0 interrupt handler:
  - Track time for prize blinking
  - Track time for gravity effect
  - Track time for score degradation

### 4. Game State Management

- Define game states:
  - Playing state
  - Soft reset state (after eating a prize)
  - Hard reset state (when hippo size reaches 6)
- Implement state transitions and logic

### 5. Hippo Movement Logic

- Implement upward movement on button press
- Implement downward movement due to gravity
- Handle boundary conditions (prevent hippo from going out of bounds)
- Handle collision detection with prize

### 6. Prize Management

- Implement prize blinking logic
- Reset prize position after it's eaten

### 7. Score Management

- Implement score calculation based on time taken to reach prize
- Update total score after each round
- Maintain score across hard resets

### 8. 7-Segment Display

- Implement functions to display digits on 7-segment displays
- Create a multiplexing routine to update all 4 displays
- Ensure displays don't flicker
- Convert score to appropriate format for display

### 9. Reset Logic

- Implement soft reset logic:
  - Blink all LEDs for 2 seconds (on/off cycle of 400ms, 3 on cycles, 2 off cycles)
  - Increase hippo size by one
  - Place hippo at bottom of screen
  - Reset prize position
- Implement hard reset logic:
  - Reset hippo size to 1 when it reaches size 6
  - Maintain total score

## Data Structures

### Game State

```c
typedef struct {
    uint8_t hippoSize;         // Size of the hippo (1-5)
    uint8_t hippoPosition;     // Position of hippo head
    uint8_t prizePosition;     // Position of the prize (typically 0)
    uint8_t prizeVisible;      // Is the prize currently visible (blinking state)
    uint16_t currentScore;     // Score for the current round (0-100)
    uint16_t totalScore;       // Total accumulated score
    uint8_t gameState;         // Current game state
    uint32_t roundStartTime;   // Time when the current round started
} GameState;
```

### Timers

```c
typedef struct {
    uint16_t gravityCounter;   // Counter for gravity tick (reset at 350ms)
    uint16_t blinkCounter;     // Counter for prize blink (reset at 500ms)
    uint16_t scoreCounter;     // Counter for score degradation (reset at 1000ms)
    uint16_t resetCounter;     // Counter for reset blinking (used during soft reset)
} Timers;
```

## Testing Strategy

1. Test TIMER0 configuration in simulator to ensure timing is accurate
2. Test button interrupt to verify hippo movement
3. Test gravity effect on hippo
4. Test collision detection with prize
5. Test score calculation and display
6. Test soft and hard reset functionality
7. Test boundary conditions and edge cases

## Implementation Timeline

1. Day 1: Setting up the project, configuration, and basic I/O
2. Day 2: Implementing TIMER0 and interrupt handlers
3. Day 3: Implementing hippo movement and gravity
4. Day 4: Implementing prize logic and collision detection
5. Day 5: Implementing score management and display
6. Day 6: Implementing reset logic
7. Day 7: Testing and debugging
8. Day 8: Final refinements and documentation

## Code Organization

1. `main.c`: Main program file, containing initialization, main loop, and interrupt handlers
2. `game.h`/`game.c`: Game logic and state management
3. `display.h`/`display.c`: 7-segment display functions
4. `timers.h`/`timers.c`: Timer management functions

## Potential Challenges

1. Timing precision for the 5ms timer interrupt
2. Debouncing the button input
3. Managing multiple timing-based events in a single interrupt handler
4. Avoiding flickering in the 7-segment display
5. Handling the soft reset blinking pattern accurately

## Final Deliverables

1. Complete C code implementation
2. Documentation of code and design choices
3. Demonstration of working game on hardware
