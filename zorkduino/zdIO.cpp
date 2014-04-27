
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

#include "ztypes.h" 

// 136 bytes total mem cache (164 bytes total to play with)
#define LINE_BITS 3   // 3 works better but cache lines no longer fit in uint16_t
#define LINE_SIZE (1 << LINE_BITS)
#define LINE_MASK ((LINE_SIZE)-1)
#define LINE_COUNT (128/LINE_SIZE)
#define EMPTY 0xFFFF

uint8_t cache_next = 0;
uint8_t cache_last = 0;
uint16_t cache_pos[LINE_COUNT];
uint8_t cache_dirty[(LINE_COUNT+7) >> 3] = {0};
uint8_t cache_data[LINE_COUNT*LINE_SIZE];

extern uint8_t sector_data[512];
void sector_read(uint16_t s);
void sector_write(uint16_t s);

class BlockCache
{
public:
    uint16_t _mark;
    uint8_t _dirty;
    BlockCache() : _mark(EMPTY),_dirty(0) {}
    
    uint8_t* find(uint32_t p)
    {
        uint16_t s = p >> 9;
        if (s != _mark)
            return 0;
        return sector_data + (p & 0x1FF);
    }

    uint8_t* seek(uint32_t p)
    {
        uint16_t s = p >> 9;
        if (_mark != s) {
            if (_dirty) {
                sector_write(_mark);
                _dirty = 0;
            }
            sector_read(s);
            _mark = s;
        }
        return sector_data + (p & 0x1FF);
    }
    
    void write(uint32_t p, uint8_t* src, uint8_t len)
    {
        uint8_t* dst = seek(p);
        while (len--)
        {
            _dirty |= *src != *dst;
            *dst++ = *src++;
        }
    }
    
    void  flush()
    {
        if (_dirty)
            sector_write(_mark);
        _dirty = 0;
        _mark = 0xFFFF;
    }
};

BlockCache blockCache;

//=======================================================================
//=======================================================================
//  Cache. Hate this code.

// cost 300 bytes. eww
#define GET_DIRTY(_n) cache_dirty[_n>>3] & (0x80 >> (_n & 7))
#define SET_DIRTY(_n) cache_dirty[_n>>3] |= (0x80 >> (_n & 7))
#define CLEAR_DIRTY(_n) cache_dirty[_n>>3] &= ~(0x80 >> (_n & 7))

#define _WRITE 1
#define _STACK 2

void cache_init()
{
    //printf("%d lines of %d, %d bytes\n",LINE_COUNT,LINE_SIZE,(int)(sizeof(cache_data) + sizeof(cache_pos)+ sizeof(cache_dirty)));
    for (uint8_t i = 0; i < LINE_COUNT; i++)
        cache_pos[i] = EMPTY;
}

void cache_flush(uint16_t sector)
{
    uint8_t* d = cache_data;
    for (uint8_t i = 0; i < LINE_COUNT; i++)
    {
        if ((cache_pos[i] != EMPTY) && (GET_DIRTY(i)))
        {
            uint32_t a = ((uint32_t)cache_pos[i]) << LINE_BITS;
            if ((a >> 9) == sector)
            {
                blockCache.write(a,d,LINE_SIZE);
                CLEAR_DIRTY(i);
            }
        }
        d += LINE_SIZE;
    }
}

void cache_flush_all()
{
    for (uint8_t i = 0; i < LINE_COUNT; i++)
    {
        if ((cache_pos[i] != EMPTY) && (GET_DIRTY(i)))
            cache_flush(cache_pos[i] >> (9 - LINE_BITS));
    }
    blockCache.flush();
}

uint8_t cache_getslot()
{
    uint8_t i;
    uint8_t write_count = 0;
    for (i = 0; i < LINE_COUNT; i++)
    {
        uint16_t cp = cache_pos[i];
        if (cp == EMPTY)
            return i;
        if (GET_DIRTY(i))
            write_count++;
    }
    write_count = write_count >= LINE_COUNT*2/3;  // Favor evicting read cache
    
    while (cache_pos[i] != EMPTY)
    {
        cache_next++;   // about as good as random..better than lru
        if (cache_next == LINE_COUNT)
            cache_next = 0;
        i = cache_next;
        if (write_count || !(GET_DIRTY(i)))
            break;
    }
    return i;
}

// return value TODO
uint8_t* cache_load(uint32_t pos, uint8_t flag = 0, zword_t value = 0)
{
    uint16_t p = pos >> LINE_BITS;
    if (!(flag & _STACK))
        p += 2048 >> LINE_BITS;

    uint8_t i = cache_last;
    uint8_t m = cache_pos[i] == p;
    if (!m) {
        for (i = 0; i < LINE_COUNT; i++)
        {
            m = cache_pos[i] == p;
            if (m)
                break;
        }
        if (!m)
            i = cache_getslot();
        cache_last = i;
    }
    
    // flush/load
    uint8_t* d = cache_data + ((uint16_t)i << LINE_BITS);
    if (!m) {
        // flush sector num of buffer we are evicting
        if (cache_pos[i] != EMPTY && (GET_DIRTY(i)))
            cache_flush(cache_pos[i] >> (9 - LINE_BITS));
        
        // fill with fresh data
        memcpy(d,blockCache.seek(((uint32_t)p) << LINE_BITS),LINE_SIZE);
        cache_pos[i] = p;
    }
    
    d += pos & LINE_MASK;
    if (flag & _WRITE)
    {
        flag = (flag & _STACK) ? *((zword_t*)d) != value : *d != value;
        if (flag & _WRITE)
            SET_DIRTY(i);
    }
    return d;
}

//=======================================================================
//=======================================================================

zbyte_t read_code_byte (void)
{
    return *cache_load((uint32_t)pc++);
}

void set_byte(zword_t a,zbyte_t value)
{
    *cache_load(a,_WRITE,value) = value;
}

zbyte_t read_data_byte(unsigned long *a)
{
    return *cache_load((uint32_t)(*a)++);
}

zword_t read_code_word (void)
{
    uint16_t h = read_code_byte();
    uint8_t l = read_code_byte();
    return (h << 8) | l;
}

zbyte_t get_byte(unsigned long a)
{
    return read_data_byte(&a);
}

zword_t get_word(unsigned long a)
{
    return read_data_word(&a);
}

void set_word(zword_t a,zword_t value)
{
    set_byte(a++,value>>8);
    set_byte(a,value);
}

zword_t read_data_word(unsigned long *a)
{
    uint16_t h = read_data_byte(a);
    uint8_t l = read_data_byte(a);
    return (h << 8) | l;
}

void PUSH(zword_t v)
{
    STACK(--sp,v);
}

zword_t POP()
{
    return STACK(sp++);
}

zword_t STACK(zword_t i)
{
    return *((zword_t*)cache_load(i<<1,_STACK));
}

void STACK(zword_t i,zword_t v)
{
    *((zword_t*)cache_load(i<<1,_STACK|_WRITE,v)) = v;
}

//=======================================================================
//=======================================================================

void undo_save (void)
{
    store_operand((zword_t) -1);
}

void undo_restore (void)
{
    store_operand((zword_t) -1);
}

//=======================================================================
//=======================================================================
//  Save and restoring games

uint8_t pad_until(uint8_t n);
void pre_input_line();

PROGMEM char s_empty[] = "[EMPTY]";
PROGMEM char s_select_save_slot[] = "Select slot for save [0..9]: ";
PROGMEM char s_select_restore_slot[] = "Select slot to restore [0..9]: ";
PROGMEM char s_saving[] = "Saving...";
PROGMEM char s_restoring[] = "Restoring...";
PROGMEM char s_wrong_game[] = "This seems to be from a different game...";
PROGMEM char s_empty_slot[] = "Can't restore from an empty slot.";

void note(const char* s)
{
  new_line();
  write_string(s);
  new_line();
}

unsigned long slot_addr(uint8_t i)
{
    unsigned long a = i;
    return (unsigned long)SAVE_REGION_OFFSET + a*(unsigned long)SAVE_SIZE;
}

// bypass line cache and use blockcache directly
zword_t saved_word(unsigned long* a)
{
    uint8_t* d = blockCache.seek(*a);
    *a += 2;
    return (d[1] << 8) | d[0];
}

void drawslot(uint8_t slot)
{
    write_char('0'+slot);
    write_char('.');
    
    unsigned long a = slot_addr(slot);
    zword_t c = 0;
    for (uint8_t i = 0; i < 12; i++)    // Steal bottom of stack to store save game info
    {
        c = saved_word(&a);
        if (c == 0)
        {
            write_string(s_empty);
            break;
        }
        write_char(c);
        write_char(c>>8);
    }
    if (c) {
        int x = saved_word(&a);
        int y = saved_word(&a);
        int config = saved_word(&a);

        if (config & CONFIG_TIME)  // TODO: Borderzone saves
            print_time(x,y);
        else
        {
            print_number(x);
            write_char('/');
            print_number(y);
        }
    }
    new_line();
}

void drawslots()
{
    new_line();
    for (int i = 0; i <= 9; i++)
    {
        write_char(' ');
        drawslot(i);
    }
    new_line();
}

int select_slot(const char* prompt)
{
    cache_flush_all();
    drawslots();
    note(prompt);
    pre_input_line();
    char c = input_character(0);
    if (c >= '0' && c <= '9')
    {
        write_char(c);
        return c-'0';
    }
    return -1;
}

int store_result(zword_t status, uint8_t v)
{
    if (h_type < V4)
        conditional_jump(status == 0);
    else
        store_operand(status == 0 ? v : 0);
    return ( status );
}

extern uint8_t _fdata[TEXT_ROWS*TEXT_COLS];

// Copy using sector buffer
void save_restore(int slot, bool sav)
{
    uint8_t n = SAVE_SIZE >> 9;
    slot = slot*n + (SAVE_REGION_OFFSET >> 9);
    n = (get_word(H_DATA_SIZE) + 511 + 2048) >> 9;
    
    cache_flush_all();
    for (int s = 0; s < n; s++)
    {
        if (sav)
        {
            sector_read(s);
            sector_write(s+slot);
        } else {
            sector_read(s+slot);
            sector_write(s);
        }
    }
}

// 1 ok
// 0 failed
int save()
{
    zword_t status = 1;
    int slot = select_slot(s_select_save_slot);
    if (slot >= 0) {
        zword_t cs = get_word(H_CHECKSUM);
        zword_t* sb = (zword_t*)_fdata;     // Copy the status bar
        uint8_t i;
        for (i = 0; i < 12; i++)            // Steal bottom of stack to store save game info
            STACK(i,*sb++ & 0x7F7F);
        STACK(i++,load_variable(17));
        STACK(i++,load_variable(18));
        STACK(i++,h_config);
        STACK(i++,cs);                      // 30
        
        STACK(i++,pc >> 16);
        STACK(i++,pc);
        STACK(i++,sp);
        STACK(i++,fp);                      // 20 stacks slots out of 1024
        
        note(s_saving);
        save_restore(slot,true);
        status = 0;
    }
    return store_result(status,1);
}

// 2 ok
// 0 failed

int restore()
{
    zword_t status = 1;
    int slot = select_slot(s_select_restore_slot);
    if (slot >= 0) {
        unsigned long a = slot_addr(slot) + 30; // checksum
        zword_t cs = saved_word(&a);
        if (cs != get_word(H_CHECKSUM)) {
            if (cs == 0)
                note(s_empty_slot);
            else
                note(s_wrong_game);
        } else {
            pc = saved_word(&a);
            pc = (pc << 16) | saved_word(&a);
            sp = saved_word(&a);
            fp = saved_word(&a);
            note(s_restoring);
            save_restore(slot,false);
            status = 0;
        }
    }
    return store_result(status,2);
}
                           
//=======================================================================
//=======================================================================
