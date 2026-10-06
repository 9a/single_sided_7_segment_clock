/*
*  This program originally was programmed for Arduino and
*  was vibe-ported to ESP home via ChatGPT.
*
*  All digits have common segment lines and are multiplexed
*  Pin assignment was optimized for single sided PCB routing
*  Check the KiCad dchematic for details.
*/

#pragma once

#include <Arduino.h>
#include <stdint.h>

// ============================================================
// ORIGINAL WIRING
//
// D4 / GPIO2 = LATCH
// D3 / GPIO0 = CLOCK
// D2 / GPIO4 = DATA
//
// No hardware SPI.
// ============================================================

#define CNC_LATCH  2
#define CNC_CLOCK  0
#define CNC_DATA   4


// ============================================================
// DISPLAY TIMING
//
// 1000 us per digit (4ms for all of the display)
//
// ============================================================

#define CNC_DIGIT_PERIOD_US 1000


// ============================================================
// ESP8266 timer1
//
// TIM_DIV16:
//
// 80 MHz / 16 = 5 MHz
//
// 1 timer tick = 200 ns
//
// ============================================================

#define CNC_TIMER_TICKS_PER_US 5


// ============================================================
// Blanking time
//
// Small OFF period around the data transfer prevents ghosting.
//
// ============================================================

#define CNC_BLANK_US 2


// ============================================================
// Segment mapping
//
// see KiCad file for pinmapping 
// ============================================================

static uint8_t cnc_segment[10] = {
    0b0001000,  // 0
    0b1011101,  // 1
    0b0010010,  // 2
    0b0010100,  // 3
    0b1000101,  // 4
    0b0100100,  // 5
    0b0100000,  // 6
    0b0011101,  // 7
    0b0000000,  // 8
    0b0000100   // 9
};


// ============================================================
// Digit selection mapping
//
// since 4 outputs of the shift registers were available
// LED anode switching was split in two separate "supplies"
// increasing the output current and thus the brightness of
// the segments
// ============================================================

static uint8_t cnc_bitmask[4] = {
    0b00000110,  // digit 0
    0b00001001,  // digit 1
    0b00110000,  // digit 2
    0b01000000   // digit 3
};


// ============================================================
// Display state
// ============================================================

volatile uint8_t cnc_digits[4] = {
    8, 8, 8, 8
};

volatile uint8_t cnc_colon = 0;

volatile uint8_t cnc_brightness = 100;

volatile uint8_t cnc_current_digit = 0;


// ============================================================
// Direct GPIO
//
// ESP8266:
//
// GPIO 0..15 are controlled through GPOS/GPOC.
//
// GPOS = set HIGH
// GPOC = set LOW
// ============================================================

IRAM_ATTR static inline void cnc_latch_low()
{
    GPOC = (1U << CNC_LATCH);
}

IRAM_ATTR static inline void cnc_latch_high()
{
    GPOS = (1U << CNC_LATCH);
}

IRAM_ATTR static inline void cnc_clock_low()
{
    GPOC = (1U << CNC_CLOCK);
}

IRAM_ATTR static inline void cnc_clock_high()
{
    GPOS = (1U << CNC_CLOCK);
}

IRAM_ATTR static inline void cnc_data_low()
{
    GPOC = (1U << CNC_DATA);
}

IRAM_ATTR static inline void cnc_data_high()
{
    GPOS = (1U << CNC_DATA);
}


// ============================================================
// Software SPI
//
// Equivalent to:
//
// shiftOut(DATA, CLOCK, LSBFIRST, value);
//
// but considerably faster because it avoids digitalWrite().
// ============================================================

IRAM_ATTR static inline void cnc_shift_byte(
    uint8_t value
)
{
    for (uint8_t i = 0; i < 8; i++) {

        if (value & 1) {
            cnc_data_high();
        } else {
            cnc_data_low();
        }

        cnc_clock_high();
        cnc_clock_low();

        value >>= 1;
    }
}


// ============================================================
// Blank the display
//
// We send both registers zero.
//
// This ensures there is no visible digit while we load the
// next 16 bits.
// ============================================================

IRAM_ATTR static inline void cnc_blank()
{
    cnc_latch_low();

    cnc_shift_byte(0);
    cnc_shift_byte(0);

    cnc_latch_high();
}


// ============================================================
// Load one digit
//
// Same byte order and bit order as your original code.
//
// ============================================================

IRAM_ATTR static inline void cnc_load_digit(
    uint8_t digit
)
{
 
	uint8_t bitm = 0;
	uint8_t first = 0;
	uint8_t second = (1 << 7);

    // Colon on digit 3.
    if (digit == 3) {
        bitm = 1;
    }
	
	if (digit < 4) {
		first =
			(bitm << 7) |
			cnc_segment[cnc_digits[digit]];
			
			
		if (cnc_colon) {
			bitm = 0;
		}

		second =
			cnc_bitmask[digit] |
			((1 ^ bitm) << 7);		
	}


    cnc_latch_low();

    cnc_shift_byte(first);
    cnc_shift_byte(second);

    cnc_latch_high();
}


// ============================================================
// Timer ISR
//
// We use two phases:
//
// PHASE 0:
//     blank
//     load next digit
//     turn digit on
//
// PHASE 1:
//     blank digit
//
// This gives PWM brightness without changing the multiplex
// frequency.
//
// only digit 0, 1, 2 and 3 cause segments to light up, so 
// having cnc_current_digit can be variably large to controll
// brightness
//
// ============================================================

volatile bool cnc_digit_on = false;

IRAM_ATTR void cnc_display_timer()
{
    //cnc_blank();

    cnc_load_digit(cnc_current_digit);

    cnc_current_digit++;

    if (cnc_current_digit >= (4 + (20 - cnc_brightness/5))) {
        cnc_current_digit = 0;
    }

    timer1_write(
        CNC_TIMER_TICKS_PER_US *
        CNC_DIGIT_PERIOD_US
    );
}


// ============================================================
// Start timer
// ============================================================

void cnc_display_setup()
{
    pinMode(CNC_LATCH, OUTPUT);
    pinMode(CNC_CLOCK, OUTPUT);
    pinMode(CNC_DATA, OUTPUT);

    digitalWrite(CNC_LATCH, HIGH);
    digitalWrite(CNC_CLOCK, LOW);
    digitalWrite(CNC_DATA, LOW);

    cnc_current_digit = 0;

    timer1_isr_init();

    timer1_attachInterrupt(cnc_display_timer);

    timer1_enable(
        TIM_DIV16,
        TIM_EDGE,
        TIM_LOOP
    );

    timer1_write(
        CNC_TIMER_TICKS_PER_US *
        CNC_DIGIT_PERIOD_US
    );
}

// ============================================================
// Update display data from normal ESPHome code
// ============================================================

void cnc_display_set_digits(
    uint8_t d0,
    uint8_t d1,
    uint8_t d2,
    uint8_t d3,
    bool colon
)
{
    noInterrupts();

    cnc_digits[0] = d0;
    cnc_digits[1] = d1;
    cnc_digits[2] = d2;
    cnc_digits[3] = d3;

    cnc_colon = colon ? 1 : 0;

    interrupts();
}


// ============================================================
// Brightness
// ============================================================

void cnc_display_set_brightness(
    uint8_t brightness
)
{
    if (brightness > 100) {
        brightness = 100;
    }

    noInterrupts();

    cnc_brightness = brightness;

    interrupts();
}