
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

//================================================================================
//================================================================================
// Zorkduino
// Play Zork on your TV with and Arduino

/*
        +----------------+           +-----------------+
        |                |           |                 |
        |             13 |-----------| SCK    micro\SD |
        |             12 |-----------| MISO   card     |
        |             11 |-----------| MOSI   module   |
        |             10 |-----------| CS              |
        |                |      5v <-|                 |
        |                |     GND <-|                 |
        |    arduino     |           +-----------------+
        |    uno/pro     |
        |                |       5v <--+-+
        |                |      GND <--|  )  TSOP38238
        |              8 |-------------+-+
        |                |
        |              6 |----[ 100k ]--------> AUDIO
        |                |
        |              9 |----[  1k  ]----+---> VIDEO
        |                |                |
        |              1 |----[ 470  ]----+
        |                |
        |              3 |----------------> *PS2 CLOCK
        |              2 |----------------> *PS2 DATA
        |                |
        +----------------+
*/

 
#include "ztypes.h"
#include "zdMmc.h"
#include "zdThin.h"

//================================================================================
//================================================================================
//  Audio feedback for keyboard and io

#define HSYNC_FREQ (1000000/63.555)
#define KEYBEEP_FREQ (HSYNC_FREQ/1000)
#define DISKBEEP_FREQ (HSYNC_FREQ/3600)

// zdVideo.cpp
void audio_beep(uint8_t freq, uint8_t cycles);
void start_video(); 
uint8_t readKey();

//================================================================================
//================================================================================

uint8_t sector_data[512];
uint32_t sector_mem_start;

// track the extnet of the stack at sector IO
// this gives a rough estimate of stack mem
//#define DEBUG_STACK
#ifdef DEBUG_STACK
prog_char _hex[] PROGMEM = "0123456789ABCDEF";
void hex(uint16_t n, uint8_t x = 0)
{
  uint8_t i = 4;
  while (i--) {
    _fdata[i+x] = pgm_read_byte(_hex+(n & 0xF));
    n >>= 4;
  }
  _fdata[4+x] = ' ';
}
volatile uint8_t* stacklo = (uint8_t*)0xFFFF;
inline void STACK_CHECK()
{
  volatile uint8_t d = 1;
  volatile uint8_t* s = &d;
  if (s < stacklo)
    stacklo = s;
}
void STACK_DUMP()
{
  hex((uint16_t)stacklo);
}
#else
#define STACK_CHECK()
#define STACK_DUMP()
#endif

uint8_t sector_write(uint16_t sector)
{
  STACK_CHECK();
  audio_beep(DISKBEEP_FREQ,16);
  return MMC_WriteSector(sector_data,sector+sector_mem_start);
}

uint8_t sector_read(uint16_t sector)
{
  STACK_CHECK();
  return MMC_ReadSector(sector_data,sector+sector_mem_start);
}

uint8_t readSector(uint8_t* data, uint32_t sector)
{
  return MMC_ReadSector(data,sector);
}

extern uint8_t cache_data[128];

PROGMEM const char s_zdmem[] = "zd.mem";
PROGMEM const char s_select_game[] = "Select game to load [0..";
PROGMEM const char s_duino[] = "      d     u     i     n     o";
PROGMEM const char s_loading[] = "    loading:";
PROGMEM const char s_rossum[] = "rossumblog.com";
PROGMEM const char c_nodisk[] = "Can't find micro/sd card";
PROGMEM const char c_no_memory[] = "Can't find zd.mem file";

extern
char* screen(uint8_t x, uint8_t y);

void draw_logo()
{
  uint8_t* d = _fdata;
  uint8_t y = 9;
  uint8_t n = 2;
  while (y--)
  {
    uint8_t x;
    for (x = 0; x < 6; x++)
      *d++ = 1;
    for (;x < 6+25; x++)
      *d++ = n++;
    for (;x < TEXT_COLS; x++)
      *d++ = 0;
  }
  strcpy_P(screen(0,9),s_duino);
  memset(screen(0,10),14,TEXT_COLS);
  memset(screen(0,12),' ',TEXT_COLS*10);
  memset(screen(0,22),14,TEXT_COLS);
  strcpy_P(screen(0,23),s_rossum);
}

typedef struct {
  int count;
  int select;
  char* name;
} FindGame;

char* name_pos(uint8_t n)
{
    uint8_t x = n & 3;
    uint8_t y = n >> 2;
    return screen(x*9+1,12+y);
}

// Display list of available games 
bool find_games(DirectoryEntry* d, int index, void* ref)
{
  char n = d->fatname[0];
  if (n != '_' && n != '.' && d->fatname[8] == 'Z' && d->fatname[10] == ' ')
  {
    n = d->fatname[9];
    if (n == '3' || n == '4' || n == '5' || n == '7')  // .z3,.z4,.z5,.z7 etc
    {
      uint8_t i;
      FindGame* fg  = (FindGame*)ref;
      if (fg->name && (fg->select == fg->count))
      {
        char* s = fg->name;
        for (i = 0; i < 8 && d->fatname[i] != ' '; i++)
          *s++ = d->fatname[i];
        *s++ = '.';
        for (i = 8; i < 11 && d->fatname[i] != ' '; i++)
          *s++ = d->fatname[i];
        *s++ = 0;
        return 1;
      }
      
      n = fg->count++;
      if (n >= 40)
        return 0;

      char* s = name_pos(n);
      for (i = 0; i < 8; i++)
        *s++ = tolower(d->fatname[i]);
    }
  }
  return 0;
}

// Select game to load
#define UP_KEY     28
#define DOWN_KEY   29
#define LEFT_KEY   30
#define RIGHT_KEY  31

void invert_name(int n)
{
  char* d = name_pos(n);
  for (int i = -1; i < 9; i++)
    d[i] ^= 0x80;
}

void select_game(Fat* fat, char* name)
{
  FindGame fg = {0};
  fat->Directory(find_games,&fg);
  if (fg.count == 1)
  {
    fg.name = name;
    fat->Directory(find_games,&fg);
    return;
  }
  
  int selected = 0;
  invert_name(0);

  for (;;) {
    uint8_t c = input_character(-1);
    int n = selected;
    switch (c) {
      case '\n':
        fg.count = 0;
        fg.select = n;
        fg.name = name;
        fat->Directory(find_games,&fg);
        return;
     case UP_KEY:     n = max(n-4,0);           break;
     case DOWN_KEY:   n = min(n+4,fg.count-1);  break;
     case LEFT_KEY:   n = max(n-1,0);           break;
     case RIGHT_KEY:  n = min(n+1,fg.count-1);  break;
    }
    if (n != selected)
    {
      invert_name(selected);
      invert_name(n);
      selected = n;
    }
  }
}

void message(const char* s)
{
  strcpy_P(screen(1,14),s);
}

// Init game by copying game zd file into zd.mem memory
int initGame()
{
  draw_logo();
  if (MMC_Init())
  {
    message(c_nodisk);
    return -1;
  }
  
  Fat* fat = (Fat*)cache_data;  // Keep it off the stack
  if (!fat->Init())
    return -2;
    
  // Open the memory file
  char buf[14];
  uint32_t startSector,fileLength;
  strcpy_P(buf,s_zdmem);
  if (!fat->Open(buf,&startSector,&fileLength))
  {
    message(c_no_memory);
    return -3;
  }
  sector_mem_start = startSector;
  uint16_t memsectors = (fileLength+511) >> 9;
    
  // Select game to load
  select_game(fat,buf);
  memset(screen(0,12),0,TEXT_COLS*10);
  
  strcpy_P(screen(0,16),s_loading);  // 20 spaces of progress
  
  if (!fat->Open(buf,&startSector,&fileLength))
    return -4;
  uint16_t gamesectors = (fileLength+511) >> 9;
  
  if (memsectors < (gamesectors + 4))
    return -5;  // No room for stack + game
    
  //  Clear stack
  memset(sector_data,0,sizeof(sector_data));
  uint16_t i;
  for (i = 0; i < 4; i++)
    sector_write(i);
    
  char* progress = screen(12,16);
  for (i = 0; i < gamesectors; i++) {
    readSector(sector_data,i+startSector);
    progress[i*20/gamesectors] = 0x80;
    sector_write(i+4);
  }
  readKey();
  return 0;
}

//================================================================================
//================================================================================

void pre_input_line()
{
  STACK_DUMP();
}

void invert_screen(bool inLogo = false)
{
  uint8_t* d = (uint8_t*)screen(0,inLogo?11:0);
  uint8_t* e = (uint8_t*)screen(0,inLogo?23:TEXT_ROWS);
  while (d < e)
  {
    d[0] ^= 0x80;
    d++;
  }
}

int input_character(int timeout)
{
  uint8_t c;
  uint8_t attract = 0;
  uint16_t start = millis()/100;
  while(!(c = readKey()))
  {
    // Timeout for borderzone?
    uint16_t elapsed = millis()/100 - start;
    if (timeout > 0 && elapsed > (uint16_t)timeout)
    {
       if (attract)
         invert_screen();
       return -1;
    }
    
    // Enter screen saver / attract mode after ~5 minutes
    uint16_t e = elapsed >> 6; // ~ 6 seconds
    if (e > (5*60/6) && ((e & 1) != attract)) // ~5 minutes
    {
      attract ^= 1;
      invert_screen(timeout == -1);
    }
  }
  audio_beep(KEYBEEP_FREQ,8);
  if (attract)
   invert_screen(timeout == -1);
  return c;
}

//==============================================================
//==============================================================

void setup()
{
  initialize_screen();
  start_video();

  while(initGame())
  {
    delay(3000);
  };

  zdInit();
}

void(*reset)(void) = 0;
void loop()
{
   zdLoop( );
   reset();
}

