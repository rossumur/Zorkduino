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

//==============================================================
//==============================================================

void initialize_screen()
{
    memset(_fdata,0,sizeof(_fdata));
    for (int x = 0; x < TEXT_COLS; x++)
        _fdata[x] = ' ' ^ 0x80;
}

void write_string( const prog_char *s );

void pre_input_line();
void write_char(int c);

int codes_to_text(int, char *)
{
    return 1;
}

int fit_line (const char *line_buffer, int pos, int mx)
{
    return (pos < mx);
}

// lighter than printf...
void print_number(zword_t num)
{
    char buffer[5];
    uint8_t i = 0;
    do
    {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    } while(num);
    while (i--)
        write_char(buffer[i]);
}

int input_line( int buflen, unsigned long addr, int timeout, int *read_size )
{
    int c;
    pre_input_line();
    
    int row,col,min_col;
    get_cursor_position (&row, &col);
    min_col = col - *read_size;

    do
    {
        c = input_character(timeout);
        if (c == -1)
            break;
        
        if (c == '\b')
        {
            if (col > min_col) {
                move_cursor (row, --col);
                display_char(' ');
                move_cursor (row, col);
                (*read_size)--;
            }
        }
        else if (c == '\n')
        {
            scroll_line();
        }
        else if ((*read_size < buflen) && (col < screen_cols))
        {
            set_byte(addr + (*read_size)++, tolower(c));
            display_char(c);
            col++;
        }
    } while (c != '\n');
    return c;
}                               /* input_line */

//==============================================================
//==============================================================

void set_font(int f)
{
}

PROGMEM const char s_Fatal[] = "Fatal:";
void fatal(uint8_t e)
{
    write_string(s_Fatal);
    print_number(e);
    pre_input_line();
    for(;;)
        ;
}

void set_colours(zword_t, zword_t)
{
}

void sound(int n, zword_t* d)
{
}

void verify()
{
}

//==============================================================
//==============================================================

static uint8_t current_row = 1;
static uint8_t current_col = 1;
static uint8_t saved_row;
static uint8_t saved_col;
static uint8_t cursor_saved = 0;
static uint8_t font_attr = 0;

char* screen(uint8_t x, uint8_t y)
{
    return (char*)_fdata + x + (int)y*TEXT_COLS;
}

void clear_text_window()
{}
void clear_status_window()
{}
void create_status_window()
{};
void delete_status_window()
{};

void clear_line (void)
{
    memset(screen(0,current_row-1),0,TEXT_COLS);
}

void clear_screen (void)
{
    memset(_fdata,0,sizeof(_fdata));
    current_row = 1;
    current_col = 1;
}

void restart_screen (void){};

void set_attribute (int a)
{
    //printf("<a%d>",a);
    font_attr = a;
};

void get_cursor_position(int* row, int* col)
{
    *row = current_row;
    *col = current_col;
}

void scroll_line()
{
    int row, col;
    //printf("\\n\n");
    get_cursor_position (&row, &col);
    move_cursor (row, 1);
    if (++current_row > screen_rows)
    {
        memcpy(screen(0,status_size),screen(0,status_size+1),TEXT_COLS*(TEXT_ROWS-(status_size+1)));
        current_row = screen_rows;
        clear_line();
    }
}

void display_char(int c)
{
    //putchar(c);
    if (font_attr & 1)
        c |= 0x80;
    *screen(current_col-1,current_row-1) = c;
    if (++current_col > screen_cols)
        current_col = screen_cols;
}

void move_cursor(int row, int col)
{
    //if (cursor_saved && col == 36)
    //    col = 1; TRINITY
    //printf("<m%d:%d>",row,col);
    current_row = row;
    current_col = col;
}

void save_cursor_position()
{
    if (cursor_saved == OFF) {
        int r,c;
        get_cursor_position (&r, &c);
        saved_row = r;
        saved_col = c;
        cursor_saved = ON;
    }
}

void restore_cursor_position()
{
    if (cursor_saved == ON) {
        move_cursor (saved_row, saved_col);
        cursor_saved = OFF;
    }
}

void select_status_window()
{
    //printf("<s>");
    save_cursor_position();
}

void select_text_window()
{
    //printf("<t>");
    restore_cursor_position();
}

int print_status (int argc, char *argv[])
{
    return (FALSE);
}

//==============================================================
//==============================================================

void configure(zbyte_t min_version, zbyte_t max_version);
void cache_init();  // zdIO.cpp

void zdInit()
{
    cache_init();
    initialize_screen();
    configure (V1, V8);
    restart();
}

void zdLoop()
{
    interpret();
}
