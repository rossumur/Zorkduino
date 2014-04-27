/* Copyright (c) 2010-2014, Peter Barrett
 **
 ** Permission to use, copy, modify, and/or distribute this software for
 ** any purpose with or without fee is hereby granted, provided that the
 ** above copyright notice and this permission notice appear in all copies.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 ** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 ** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
 ** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 ** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 ** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 ** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 ** SOFTWARE.
 */

// Video output, audio tone generation and IR keyboard scanning events

#include "ztypes.h"

__attribute__((section(".progmem.data")))
const unsigned char atascii[] = {
#include "zdFont.h"  // SWIZZLED ATARI FONT
#include "zdLogofont.h"  // Zork logo
};

// PORT B
#define SYNC_PIN    1
#define IR_PIN      0

// PORT D
#define AUDIO_PIN       6
#define PS2_CLOCK_PIN   3
#define PS2_DATA_PIN    2
#define VIDEO_PIN       1

// ~1016 cpu clocks per line at 16Mhz
#define USEC(_x)         (((_x)*F_CPU/1000000)-1)
#define TICKS_SCANLINE   USEC(63.555)
#define TICKS_HSYNC      USEC(4.7)
#define TICKS_LONG_HSYNC USEC(63.555-4.7)
#define TICKS_HBLANK     USEC(10.30)

#define BLANK_LINES 40
#define ACTIVE_LINES (TEXT_ROWS*8)

#define STATE_VBLANK  0
#define STATE_PRE     1
#define STATE_ACTIVE  2
#define STATE_POST    3

// does not do halfline sync during blanking
__attribute__((section(".progmem.data")))
prog_char v_lines[] = { 3,BLANK_LINES,ACTIVE_LINES,262-(3+BLANK_LINES+ACTIVE_LINES) };

// Video State
uint8_t v_state = 0;
uint8_t v_count = 0;
uint8_t v_vbicountdown = 0;
uint8_t _fdata[TEXT_COLS*TEXT_ROWS];    // Framebuffer

// Audio state
uint8_t a_count = 0;
uint8_t a_freq = 0;
uint8_t a_cycles = 0;

// IR state
uint8_t ir_last = 0;
uint8_t ir_count = 0;

//==========================================================
//==========================================================
//  Webtv keyboard

// Report changes on IR pin to keyboard handler
// t is HSYNCH ticks
void keyboardIREvent(uint8_t t, uint8_t v); 

//==========================================================
//==========================================================
// PS2 keyboard

void enable_ps2();
void disable_ps2();

//==========================================================
//==========================================================
//  audio for keyboard/io feedback

void audio_beep(uint8_t freq, uint8_t cycles)
{
  a_freq = freq;
  a_cycles = cycles;
  a_count = freq;
}

//==========================================================
//==========================================================
//  Video ISR

static void inline tick_align(uint8_t tick)
{
  __asm__ __volatile__ (
      "sub	%[tick], %[tcnt1l]\n"
"loop: subi	%[tick], 3\n"
      "brcc	loop\n"
      "cpi      %[tick],-3\n"
      "breq     done\n"
      "cpi      %[tick],-2\n"
      "brne     .\n"
"done:\n"
      :: [tick] "a" (tick),
      [tcnt1l] "a" (TCNT1L)
    );
}

// Video ISR
// Handles video and audio generation as well as keyboard scanning
ISR(TIMER1_OVF_vect)
{
  uint8_t ir = PINB & 1;
  OCR1A = v_state ? TICKS_HSYNC : TICKS_LONG_HSYNC;
  
  if (v_state == STATE_ACTIVE)
  {
    UDR0 = 0;
    UCSR0C = (1<<USBS0)|(3<<UCSZ00);
    tick_align(TICKS_HBLANK);
    UCSR0B = 1<<TXEN0;

    uint8_t line = ACTIVE_LINES-v_count;
    uint8_t* src = _fdata + (line >> 3)*TEXT_COLS;
    uint16_t font = (uint16_t)atascii + ((line&7) << 8); // + fontv*TEXT_COLS

    // If a line starts with a 1, use the logofont
    uint16_t n = src[0] == 1;
    font += n<<11;

    // Delay by some chars to center text
    uint8_t a,i,c;
    a = 2;
    while (a--) {
        UDR0 = 0;
        i = 4;
        while (i--)
            ;
    }
    UCSR0C = (1<<UMSEL01)|(1<<UMSEL00)|(0<<UCPHA0)|(0<<UCPOL0); // START SPI MODE
    
    // 40 character output
    i = 0;
    a = TEXT_COLS;
    while (a--)
    {
        c = src[i++];  // zero byte line ending - early out. Simple CPU recovery strategy.
        if (!c)
          break;
        UDR0 = pgm_read_byte(font+c);
        asm("nop");
    }

    UDR0 = 0;
    UCSR0B = 0;
  }

  // Advance video state machine
  if (!--v_count)
  {
    v_state = (v_state+1) & 3;
    v_count = pgm_read_byte(v_lines + v_state);
    if (v_state == STATE_ACTIVE)
    {
      disable_ps2();         // don't field ps2 keyboard interrupts during active video
      if (v_vbicountdown)
          v_vbicountdown--;  // VBI countdown timer
    }
    else if (v_state == STATE_POST)
        enable_ps2();
  }

  // ir keyboard
  if (ir != ir_last)
  {
    keyboardIREvent(ir_count,ir_last);
    ir_count = 0;
    ir_last = ir;
  }
  if (ir_count != 0xFF)
    ir_count++;
    
  // audio out
  if (a_cycles && !--a_count)
  {
    --a_cycles;
    a_count = a_freq;
    PORTD ^= _BV(6);
  }
}

void start_video()
{
    UBRR0H = 0;  // Fastest serial clock  
    UBRR0L = 0;
        
    DDRB |= _BV(SYNC_PIN);
    DDRB &= ~_BV(IR_PIN);
    DDRD |= _BV(AUDIO_PIN) | _BV(VIDEO_PIN);
    PORTD &= ~_BV(VIDEO_PIN);  // black when serial port is not driving TX

    // Set OC1A on Compare Match, Fast PWM, No prescaling, Timer 1
    TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
    ICR1 = TICKS_SCANLINE;
    OCR1A = TICKS_HSYNC; 
    TIMSK1 = _BV(TOIE1);
    PORTD = 0;
    v_state = 3;
    v_count = 1;
    sei();
}

