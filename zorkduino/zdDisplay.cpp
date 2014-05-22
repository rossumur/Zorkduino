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

//=================================================================
//=================================================================
// the following are present in the font at chars 16-23
// ëïéà èùâê îôûç

// see http://www.gnelson.demon.co.uk/zspec/sect03.html#five section 3.8.7

enum {
    F_E_DIAERESIS = 16,
    F_I_DIAERESIS,
    F_E_ACUTE,
    F_A_GRAVE,
    
    F_E_GRAVE,
    F_U_GRAVE,
    F_A_CURCUMFLEX,
    F_E_CURCUMFLEX,
    
    F_I_CURCUMFLEX,
    F_O_CURCUMFLEX,
    F_U_CURCUMFLEX,
    F_C_CEDILLA
};

// font defined chars
#define Z_E_DIAERESIS 164 // ë
#define Z_I_DIAERESIS 165 // ï
//#define Z_Y_DIAERESIS 166

//#define Z_A_ACUTE 169
#define Z_E_ACUTE 170       // é

#define Z_A_GRAVE 181       // à
#define Z_E_GRAVE 182       // è
//#define Z_I_GRAVE 183
//#define Z_O_GRAVE 184
#define Z_U_GRAVE 185      // ù

#define Z_A_CURCUMFLEX 191 // â
#define Z_E_CURCUMFLEX 192 // ê
#define Z_I_CURCUMFLEX 193 // î
#define Z_O_CURCUMFLEX 194 // ô
#define Z_U_CURCUMFLEX 195 // û

#define Z_C_CEDILLA 213 // ç

const char c155_233[] PROGMEM = {
    'a','e',
    'o','e',
    'u','e',
    'A','e',
    'O','e',
    'U','e',
    's','s',
    '>','>',
    '<','<',
    
    F_E_DIAERESIS,0,  // 164	 0eb	e-diaeresis	e
    F_I_DIAERESIS,0,  // 165	 0ef	i-diaeresis	i
    'y',0,            // 166	 0ff	y-diaeresis	y
    'E',0, // 167	 0cb	E-diaeresis	E
    'I',0, // 168	 0cf	I-diaeresis	I
    'a',0, // 169	 0e1	a-acute	a
    F_E_ACUTE,0,//170	 0e9	e-acute	e
    'i',0, // 171	 0ed	i-acute	i
    'o',0, // 172	 0f3	o-acute	o
    'u',0, // 173	 0fa	u-acute	u
    'y',0, // 174	 0fd	y-acute	y
    'A',0, // 175	 0c1	A-acute	A
    'E',0, // 176	 0c9	E-acute	E
    'I',0, // 177	 0cd	I-acute	I
    'O',0, // 178	 0d3	O-acute	O
    'U',0, // 179	 0da	U-acute	U
    'Y',0, // 180	 0dd	Y-acute	Y
    F_A_GRAVE,0,//181	 0e0	a-grave	a
    F_E_GRAVE,0,//182	 0e8	e-grave	e
    'i',0, // 183	 0ec	i-grave	i
    'o',0, // 184	 0f2	o-grave	o
    F_U_GRAVE,0,// 185	 0f9	u-grave	u
    'A',0, // 186	 0c0	A-grave	A
    'E',0, // 187	 0c8	E-grave	E
    'I',0, // 188	 0cc	I-grave	I				
    'O',0, // 189	 0d2	O-grave	O				
    'U',0, // 190	 0d9	U-grave	U
    
    F_A_CURCUMFLEX,0,// 191	 0e2	a-circumflex	a
    F_E_CURCUMFLEX,0, // 192	 0ea	e-circumflex	e
    F_I_CURCUMFLEX,0, // 193	 0ee	i-circumflex	i
    F_O_CURCUMFLEX,0, // 194	 0f4	o-circumflex	o
    F_U_CURCUMFLEX,0, // 195	 0fb	u-circumflex	u
    'A',0, // 196	 0c2	A-circumflex	A
    'E',0, // 197	 0ca	E-circumflex	E
    'I',0, // 198	 0ce	I-circumflex	I
    'O',0, // 199	 0d4	O-circumflex	O
    'U',0, //  200	 0db	U-circumflex	U
    
    'a',0, //  201	 0e5	a-ring	a
    'A',0, //  202	 0c5	A-ring	A
    'o',0, //  203	 0f8	o-slash	o
    'O',0, //  204	 0d8	O-slash	O
    'a',0, //  205	 0e3	a-tilde	a
    'n',0, //  206	 0f1	n-tilde	n
    'o',0, //  207	 0f5	o-tilde	o
    'A',0, //  208	 0c3	A-tilde	A
    'N',0, //  209	 0d1	N-tilde	N
    'O',0, //  210	 0d5	O-tilde	O

    'a','e',
    'A','E',
    F_C_CEDILLA,0,
    't','h',
    't','h',
    't','h',
    'T','h',
    'T','h',
    'L',0,
    'o','e',
    'O','E',
    '!',0,
    '?',0
};

// Map codes to special extra characters or nearest default translation 
int codes_to_text(int c, char *d)
{
    if (c < 155 || c > 233)
        return 1;
    const char* p = c155_233 + ((c - 155) << 1);
    d[0] = pgm_read_byte(p++);
    d[1] = pgm_read_byte(p);
    d[2] = 0;
    return 0;
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
    
    // Upgrade 0 to ' '
    char* c = screen(current_col-1,current_row-1);
    uint8_t n = current_col-1;
    while (n--)
    {
        if (!*--c)
            *c = ' ';
    }
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
