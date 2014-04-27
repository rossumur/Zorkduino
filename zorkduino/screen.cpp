/*
 * screen.c
 *
 * Generic screen manipulation routines. Most of these routines call the machine
 * specific routines to do the actual work.
 *
 */

#include "ztypes.h"

/*
 * select_window
 *
 * Put the cursor in the text or status window. The cursor is free to move in
 * the status window, but is fixed to the input line in the text window.
 *
 */

uint8_t font = 1;
uint8_t formatting = ON;
uint8_t replaying = 0;
uint8_t outputting = 0;
uint8_t lines_written = 0;

char foo[screen_cols+1] = {0};
char* line = foo;
char* status_line = 0;
uint8_t status_pos = 0;

#ifdef __STDC__
void select_window (zword_t w)
#else
void select_window (w)
zword_t w;
#endif
{
    int row, col;

    flush_buffer (FALSE);

    screen_window = w;

    if (screen_window == STATUS_WINDOW) {

        /* Status window: disable formatting and select status window */

        formatting = OFF;
        //scripting_disable = ON;
        select_status_window ();

        /* Put cursor at top of status area */

        if (h_type < V4)
            move_cursor (2, 1);
        else
            move_cursor (1, 1);

    } else {

        /* Text window: enable formatting and select text window */

        select_text_window ();
        //scripting_disable = OFF;
        formatting = ON;

        /* Move cursor if it has been left in the status area */

        get_cursor_position (&row, &col);
        if (row <= status_size)
            move_cursor (status_size + 1, 1);

    }

    /* Force text attribute to normal rendition */

    set_attribute (NORMAL);

}/* select_window */

/*
 * set_status_size
 *
 * Set the size of the status window. The default size for the status window is
 * zero lines for both type 3 and 4 games. The status line is handled specially
 * for type 3 games and always occurs the line immediately above the status
 * window.
 *
 */

#ifdef __STDC__
void set_status_size (zword_t lines)
#else
void set_status_size (lines)
zword_t lines;
#endif
{

    /* Maximum status window size is 255 */

    lines &= 0xff;

    /* The top line is always set for V1 to V3 games, so account for it here. */

    if (h_type < V4)
        lines++;

    if (lines) {

        /* If size is non zero the turn on the status window */

        status_active = ON;

        /* Bound the status size to one line less than the total screen height */

        if (lines > (zword_t) (screen_rows - 1))
            status_size = (zword_t) (screen_rows - 1);
        else
            status_size = lines;

        /* Create the status window, or resize it */

        create_status_window ();

        /* Need to clear the status window for type 3 games */

        if (h_type < V4)
            erase_window (STATUS_WINDOW);

    } else {

        /* Lines are zero so turn off the status window */

        status_active = OFF;

        /* Reset the lines written counter and status size */

        lines_written = 0;
        status_size = 0;

        /* Delete the status window */

        delete_status_window ();

        /* Return cursor to text window */

        select_text_window ();
    }

}/* set_status_size */

/*
 * erase_window
 *
 * Clear one or all windows on the screen.
 *
 */

#ifdef __STDC__
void erase_window (zword_t w)
#else
void erase_window (w)
zword_t w;
#endif
{

    flush_buffer (TRUE);

    if ((zbyte_t) w == (zbyte_t) SCREEN) {
        clear_screen ();
    } else if ((zbyte_t) w == TEXT_WINDOW) {
        clear_text_window ();
    } else if ((zbyte_t) w == STATUS_WINDOW) {
        clear_status_window ();
        return;
    }

    if (h_type > V4)
        move_cursor (1, 1);
    else
        move_cursor (screen_rows, 1);

}/* erase_window */

/*
 * erase_line
 *
 * Clear one line on the screen.
 *
 */

#ifdef __STDC__
void erase_line (zword_t flag)
#else
void erase_line (flag)
zword_t flag;
#endif
{

    if (flag == TRUE)
        clear_line ();

}/* erase_line */

/*
 * set_cursor_position
 *
 * Set the cursor position in the status window only.
 *
 */

#ifdef __STDC__
void set_cursor_position (zword_t row, zword_t column)
#else
void set_cursor_position (row, column)
zword_t row;
zword_t column;
#endif
{

    /* Can only move cursor if format mode is off and in status window */

    if (formatting == OFF && screen_window == STATUS_WINDOW)
        move_cursor (row, column);

}/* set_cursor_position */

/*
 * pad_line
 *
 * Pad the status line with spaces up to a column position.
 *
 */
#if 0

#ifdef __STDC__
static void pad_line (int column)
#else
static void pad_line (column)
int column;
#endif
{
    int i;

    for (i = status_pos; i < column; i++)
        write_char (' ');
    status_pos = column;

}/* pad_line */
#endif

/*
 * display_status_line
 *
 * Format and output the status line for type 3 games only.
 *
 */

extern
uint8_t* screen(uint8_t x, uint8_t y);

PROGMEM const char s_Time[] = "Time: ";
PROGMEM const char s_Score[] = "Score: ";

#ifdef __STDC__
void display_status_line (void)
#else
void display_status_line ()
#endif
{

    /* Move the cursor to the top line of the status window, set the reverse
       rendition and print the status line */

    select_window (STATUS_WINDOW);
    move_cursor (1, 1);
    set_attribute (REVERSE);

    /* Redirect output to the status line buffer */

    status_line = (char*)screen(0,0);
    set_print_modes (3, 0);

    /* Print the object description for global variable 16 */

    blank_status_line();
    
    //pad_line (1);
    //status_part[count] = &status_line[status_pos];
    if (load_variable (16) != 0)
        print_object (load_variable (16));
    //end_of_string[count++] = status_pos;
    //status_line[status_pos++] = '\0';

    if (get_byte (H_CONFIG) & CONFIG_TIME) {
        status_pos = 18;
        /* If a time display print the hours and minutes from global
           variables 17 and 18 */

        //status_part[count] = &status_line[status_pos];
        write_string(s_Time);
        print_time(load_variable (17), load_variable (18));
        //end_of_string[count++] = status_pos;
        //status_line[status_pos++] = '\0';
    } else {
        status_pos = 24;

        /* If a moves/score display print the score and moves from global
           variables 17 and 18 */
/*
        pad_line (screen_cols - 31);
        status_part[count] = &status_line[status_pos];
        write_string (" Score: ");
        print_number (load_variable (17));
        end_of_string[count++] = status_pos;
        status_line[status_pos++] = '\0';

        pad_line (screen_cols - 15);
        status_part[count] = &status_line[status_pos];
        write_string (" Moves: ");
        print_number (load_variable (18));
        end_of_string[count++] = status_pos;
        status_line[status_pos++] = '\0';
 */
        //status_part[count] = &status_line[status_pos];
        write_string (s_Score);
        print_number (load_variable (17));
        status_line[status_pos++] = '/' | 0x80;
        print_number (load_variable (18));
    }

    /* Pad the end of status line with spaces then disable output redirection */

    //pad_line (screen_cols);
    set_print_modes ((zword_t) -3, 0);

    /* Try and print the status line for a proportional font screen. If this
       fails then remove embedded nulls in status line buffer and just output
       it to the screen */
    
/*
    if (print_status (count, status_part) == FALSE) {
        for (i = 0; i < count; i++)
            status_line[end_of_string[i]] = ' ';
        status_line[status_pos] = '\0';
        write_string (status_line);
    }
*/
    status_line = 0;

    set_attribute (NORMAL);
    select_window (TEXT_WINDOW);

}/* display_status_line */

/*
 * blank_status_line
 *
 * Output a blank status line for type 3 games only.
 *
 */

#ifdef __STDC__
void blank_status_line (void)
#else
void blank_status_line ()
#endif
{
    memset(screen(0,0),' ' ^ 0x80,screen_cols);
#if 0
    /* Move the cursor to the top line of the status window, set the reverse
       rendition and print the status line */

    select_window (STATUS_WINDOW);
    move_cursor (1, 1);
    set_attribute (REVERSE);
    
    /* Redirect output to the status line buffer and pad the status line with
       spaces then disable output redirection */

    set_print_modes (3, 0);
    pad_line (screen_cols);
    status_line[status_pos] = '\0';
    set_print_modes ((zword_t) -3, 0);

    /* Write the status line */

    write_string (status_line);
    /* Turn off attributes and return to text window */

    set_attribute (NORMAL);
    select_window (TEXT_WINDOW);
#endif

}/* blank_status_line */

/*
 * output_string
 *
 * Output a string of characters.
 *
 */

#ifdef __STDC__
void output_string (const char *s)
#else
void output_string (s)
const char *s;
#endif
{

    while (*s)
        output_char (*s++);

}/* output_string */

/*
 * output_line
 *
 * Output a string of characters followed by a new line.
 *
 */

#ifdef __STDC__
void output_line (const char *s)
#else
void output_line (s)
const char *s;
#endif
{

    output_string (s);
    output_new_line ();

}/* output_line */

/*
 * output_char
 *
 * Output a character and rendition selection. This routine also handles
 * selecting rendition attributes such as bolding and reverse. There are
 * five attributes distinguished by a bit mask. 0 means turn all attributes
 * off. The attributes are: 1 = reverse, 2 = bold, 4 = emphasis, and
 * 8 = fixed font.
 *
 */

#ifdef __STDC__
void output_char (int c)
#else
void output_char (c)
int c;
#endif
{

    /* If output is enabled then either select the rendition attribute
       or just display the character */

    if (outputting == ON) {

        /* Make sure we are dealing with a positive integer */

        c = (unsigned int) (c & 0xff);

        /* Attribute selection */

        if (c >= (MIN_ATTRIBUTE + 1) && c <= (MAX_ATTRIBUTE + 1)) {

            set_attribute (--c);

        } else {

            display_char (c);

        }
    }

}/* output_char */

/*
 * output_new_line
 *
 * Scroll the text window up one line and pause the window if it is full.
 *
 */

PROGMEM const char s_more[] = "[MORE]";
#ifdef __STDC__
void output_new_line (void)
#else
void output_new_line ()
#endif
{
    int row, col;

    /* Don't print if output is disabled or replaying commands */

    if (outputting == ON) {

        if (formatting == ON && screen_window == TEXT_WINDOW) {

            /* If this is the text window then scroll it up one line */

            scroll_line ();

            /* See if we have filled the screen. The spare line is for the [MORE] message */

            if (++lines_written >= ((screen_rows - top_margin) - status_size - 1)) {

                /* Display the new status line while the screen in paused */

                if (h_type < V4)
                    display_status_line ();

                /* Reset the line count and display the more message */

                lines_written = 0;

                if (replaying == OFF) {
                    get_cursor_position (&row, &col);
                    char buf[8];
                    strcpy_P(buf,s_more);
                    output_string (buf);
                    (void) input_character (0);
                    move_cursor (row, col);
                    clear_line ();
                }
            }
        } else

            /* If this is the status window then just output a new line */

            output_char ('\n');
    }

}/* output_new_line */

/*
 * print_window
 *
 * Writes text into a rectangular window on the screen.
 *
 *    argv[0] = start of text address
 *    argv[1] = rectangle width
 *    argv[2] = rectangle height (default = 1)
 *
 */

#ifdef __STDC__
void print_window (int argc, zword_t *argv)
#else
void print_window (argc, argv)
int argc;
zword_t *argv;
#endif
{
    unsigned long address;
    unsigned int width, height;
    unsigned int row, column;

    /* Supply default arguments */

    if (argc < 3)
        argv[2] = 1;

    /* Don't do anything if the window is zero high or wide */

    if (argv[1] == 0 || argv[2] == 0)
        return;

    /* Get coordinates of top left corner of rectangle */

    get_cursor_position ((int *) &row, (int *) &column);

    address = argv[0];

    /* Write text in width * height rectangle */

    for (height = 0; height < argv[2]; height++) {

        for (width = 0; width < argv[1]; width++)
            write_char (read_data_byte (&address));

        /* Put cursor back to lefthand side of rectangle on next line */

        if (height != (argv[2] - 1))
            move_cursor (++row, column);

    }

}/* print_window */

/*
 * set_font_attribute
 *
 * Set text or graphic font. 1 = text font, 3 = graphics font.
 *
 */

#ifdef __STDC__
void set_font_attribute (zword_t new_font)
#else
void set_font_attribute (new_font)
zword_t new_font;
#endif
{
    zword_t old_font = font;

    if (new_font != old_font) {
        font = new_font;
        set_font (font);
    }

    store_operand (old_font);

}/* set_font_attribute */

/*
 * set_colour_attribute
 *
 * Set the colour of the screen. Colour can be set on four things:
 *    Screen background
 *    Text typed by player
 *    Text written by game
 *    Graphics characters
 *
 * Colors can be set to 1 of 9 values:
 *    1 = machine default (IBM/PC = blue background, everything else white)
 *    2 = black
 *    3 = red
 *    4 = green
 *    5 = brown
 *    6 = blue
 *    7 = magenta
 *    8 = cyan
 *    9 = white
 *
 */

#ifdef __STDC__
void set_colour_attribute (zword_t foreground, zword_t background)
#else
void set_colour_attribute (foreground, background)
zword_t foreground;
zword_t background;
#endif
{

/*
    if ((short)foreground < -1 || (short)foreground > 9 ||
	(short)background < -1 || (short)background > 9)
	fatal ("Bad colour!");

*/
    flush_buffer (FALSE);

    set_colours (foreground, background);

    return;

}/* set_colour_attribute */

