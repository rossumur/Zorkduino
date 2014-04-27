/*
 * text.c
 *
 * Text manipulation routines
 *
 */

#include "ztypes.h"


static uint8_t saved_formatting = ON;
static int story_buffer = 0;
static int story_pos = 0;
static int story_count = 0;
static uint8_t line_pos = 0;

/*
 * decode_text
 *
 * Convert encoded text to ASCII. Text is encoded by squeezing each character
 * into 5 bits. 3 x 5 bit encoded characters can fit in one word with a spare
 * bit left over. The spare bit is used to signal to end of a string. The 5 bit
 * encoded characters can either be actual character codes or prefix codes that
 * modifier the following code.
 *
 */

#ifdef __STDC__
void decode_text (unsigned long *address)
#else
void decode_text (address)
unsigned long *address;
#endif
{
    int i, synonym_flag, synonym = 0, ascii_flag, ascii = 0;
    int data, code, shift_state, shift_lock;
    unsigned long addr;

    /* Set state variables */

    shift_state = 0;
    shift_lock = 0;
    ascii_flag = 0;
    synonym_flag = 0;

    do {

        /*
         * Read one 16 bit word. Each word contains three 5 bit codes. If the
         * high bit is set then this is the last word in the string.
         */

        data = read_data_word (address);

        for (i = 10; i >= 0; i -= 5) {

            /* Get code, high bits first */

            code = (data >> i) & 0x1f;

            /* Synonym codes */

            if (synonym_flag) {

                synonym_flag = 0;
                synonym = (synonym - 1) * 64;
                addr = (unsigned long) get_word (h_synonyms_offset + synonym + (code * 2)) * 2;
                decode_text (&addr);
                shift_state = shift_lock;

            /* ASCII codes */

            } else if (ascii_flag) {

                /*
                 * If this is the first part ASCII code then remember it.
                 * Because the codes are only 5 bits you need two codes to make
                 * one eight bit ASCII character. The first code contains the
                 * top 3 bits. The second code contains the bottom 5 bits.
                 */

                if (ascii_flag++ == 1)

                    ascii = code << 5;

                /*
                 * If this is the second part ASCII code then assemble the
                 * character from the two codes and output it.
                 */

                else {

                    ascii_flag = 0;
                    write_zchar ((char) (ascii | code));

                }

            /* Character codes */

            } else if (code > 5) {

                code -= 6;

                /*
                 * If this is character 0 in the punctuation set then the next two
                 * codes make an ASCII character.
                 */

                if (shift_state == 2 && code == 0)

                    ascii_flag = 1;

                /*
                 * If this is character 1 in the punctuation set then this
                 * is a new line.
                 */

                else if (shift_state == 2 && code == 1 && h_type > V1)

                    new_line ();

                /*
                 * This is a normal character so select it from the character
                 * table appropriate for the current shift state.
                 */

                else

                    write_zchar (lookup_table(shift_state,code));

                shift_state = shift_lock;

            /* Special codes 0 to 5 */

            } else {

                /*
                 * Space: 0
                 *
                 * Output a space character.
                 *
                 */

                if (code == 0) {

                    write_zchar (' ');

                } else {

                    /*
                     * The use of the synonym and shift codes is the only difference between
                     * the different versions.
                     */

                    if (h_type < V3) {

                        /*
                         * Newline or synonym: 1
                         *
                         * Output a newline character or set synonym flag.
                         *
                         */

                        if (code == 1) {

                            if (h_type == V1)

                                new_line ();

                            else {

                                synonym_flag = 1;
                                synonym = code;

                            }

                        /*
                         * Shift keys: 2, 3, 4 or 5
                         *
                         * Shift keys 2 & 3 only shift the next character and can be used regardless of
                         * the state of the shift lock. Shift keys 4 & 5 lock the shift until reset.
                         *
                         * The following code implements the the shift code state transitions:
                         *
                         *               +-------------+-------------+-------------+-------------+
                         *               |       Shift   State       |        Lock   State       |
                         * +-------------+-------------+-------------+-------------+-------------+
                         * | Code        |      2      |       3     |      4      |      5      |
                         * +-------------+-------------+-------------+-------------+-------------+
                         * | lowercase   | uppercase   | punctuation | uppercase   | punctuation |
                         * | uppercase   | punctuation | lowercase   | punctuation | lowercase   |
                         * | punctuation | lowercase   | uppercase   | lowercase   | uppercase   |
                         * +-------------+-------------+-------------+-------------+-------------+
                         *
                         */

                        } else {
                            if (code < 4)
                                shift_state = (shift_lock + code + 2) % 3;
                            else
                                shift_lock = shift_state = (shift_lock + code) % 3;
                        }

                    } else {

                        /*
                         * Synonym table: 1, 2 or 3
                         *
                         * Selects which of three synonym tables the synonym
                         * code following in the next code is to use.
                         *
                         */

                        if (code < 4) {

                            synonym_flag = 1;
                            synonym = code;

                        /*
                         * Shift key: 4 or 5
                         *
                         * Selects the shift state for the next character,
                         * either uppercase (4) or punctuation (5). The shift
                         * state automatically gets reset back to lowercase for
                         * V3+ games after the next character is output.
                         *
                         */

                        } else {

                            shift_state = code - 3;
                            shift_lock = 0;

                        }
                    }
                }
            }
        }
    } while ((data & 0x8000) == 0);

}/* decode_text */

/*
 * encode_text
 *
 * Pack a string into up to 9 codes or 3 words.
 *
 */

void encode_text (int len, zword_t s, short *buffer)
{
    int i, j, prev_table, table, next_table, shift_state, code, codes_count;
    char codes[9];

    /* Initialise codes count and prev_table number */

    codes_count = 0;
    prev_table = 0;

    /* Scan do the string one character at a time */

    while (len--) {

        /*
         * Set the table and code to be the ASCII character inducer, then
         * look for the character in the three lookup tables. If the
         * character isn't found then it will be an ASCII character.
         */
        
        zbyte_t s0 = get_byte(s);
        zbyte_t s1 = get_byte(s+1);

        table = 2;
        code = 0;
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 26; j++) {
                if (lookup_table(i,j) == s0) {
                    table = i;
                    code = j;
                }
            }
        }

        /*
         * Type 1 and 2 games differ on how the shift keys are used. Switch
         * now depending on the game version.
         */

        if (h_type < V3) {

            /*
             * If the current table is the same as the previous table then
             * just store the character code, otherwise switch tables.
             */

            if (table != prev_table) {

                /* Find the table for the next character */

                next_table = 0;
                if (len) {
                    next_table = 2;
                    for (i = 0; i < 3; i++) {
                        for (j = 0; j < 26; j++) {
                            if (lookup_table(i,j) == s1)
                                next_table = i;
                        }
                    }
                }

                /*
                 * Calculate the shift key. This magic. See the description in
                 * decode_text for more information on version 1 and 2 shift
                 * key changes.
                 */

                shift_state = (table + (prev_table * 2)) % 3;

                /* Only store the shift key if there is a change in table */

                if (shift_state) {

                    /*
                     * If the next character as the uses the same table as
                     * this character then change the shift from a single
                     * shift to a shift lock. Also remember the current
                     * table for the next iteration.
                     */

                    if (next_table == table) {
                        shift_state += 2;
                        prev_table = table;
                    } else
                        prev_table = 0;

                    /* Store the code in the codes buffer */

                    if (codes_count < 9)
                        codes[codes_count++] = (char) (shift_state + 1);
                }
            }
        } else {

            /*
             * For V3 games each uppercase or punctuation table is preceded
             * by a separate shift key. If this is such a shift key then
             * put it in the codes buffer.
             */

            if (table && codes_count < 9)
                codes[codes_count++] = (char) (table + 3);
        }

        /* Put the character code in the code buffer */

        if (codes_count < 9)
            codes[codes_count++] = (char) (code + 6);

        /*
         * Cannot find character in table so treat it as a literal ASCII
         * code. The ASCII code inducer (code 0 in table 2) is followed by
         * the high 3 bits of the ASCII character followed by the low 5
         * bits to make 8 bits in total.
         */

        if (table == 2 && code == 0) {
            if (codes_count < 9)
                codes[codes_count++] = (char) ((s0 >> 5) & 0x07);
            if (codes_count < 9)
                codes[codes_count++] = (char) (s0 & 0x1f);
        }

        /* Advance to next character */

        s++;

    }

    /* Pad out codes with shift 5's */

    while (codes_count < 9)
        codes[codes_count++] = 5;

    /* Pack codes into buffer */

    buffer[0] = ((short) codes[0] << 10) | ((short) codes[1] << 5) | (short) codes[2];
    buffer[1] = ((short) codes[3] << 10) | ((short) codes[4] << 5) | (short) codes[5];
    buffer[2] = ((short) codes[6] << 10) | ((short) codes[7] << 5) | (short) codes[8];

    /* Terminate buffer at 6 or 9 codes depending on the version */

    if (h_type < V4)
        buffer[1] |= 0x8000;
    else
        buffer[2] |= 0x8000;

}/* encode_text */

/*
 * write_zchar
 *
 * High level Z-code character output routine. Translates Z-code characters to
 * machine specific character(s) before output. If it cannot translate it then
 * use the default translation. If the character is still unknown then display
 * a '?'.
 *
 */

#ifdef __STDC__
void write_zchar (int c)
#else
void write_zchar (c)
int c;
#endif
{
    char xlat_buffer[MAX_TEXT_SIZE + 1], xlat[18];
    int i;

    c = (unsigned int) (c & 0xff);

    /* If character is not special character then just write it */

    if (c <= '~' && (c < 24 || c > 27)) {

        write_char (c);

    } else {

        /* Put default character in translation buffer */

        xlat_buffer[0] = '?';
        xlat_buffer[1] = '\0';

        /* If translation fails then supply a default */

        if (codes_to_text (c, xlat_buffer)) {

            /* Arrow keys - these must the keyboard keys used for input */

            if (c > 23 && c < 28) {
/*  Changed because the following won't compile on non-ANSI compilers */
/*              char xlat[4] = { '\\', '/', '+', '-' };
*/
                xlat[0] = '\\';
                xlat[1] = '/';
                xlat[2] = '+';
                xlat[3] = '-';

                xlat_buffer[0] = xlat[c - 24];
                xlat_buffer[1] = '\0';
            }

            /* IBM line drawing characters to ASCII characters */

            if (c > 178 && c < 219) {

                if (c == 179)
                    xlat_buffer[0] = '|';
                else if (c == 186)  
                    xlat_buffer[0] = '#';
                else if (c == 196)
                    xlat_buffer[0] = '-';
                else if (c == 205)
                    xlat_buffer[0] = '=';
                else
                    xlat_buffer[0] = '+';
                xlat_buffer[1] = '\0';
            }

            /* German character replacements */

            if (c > 154 && c < 164) {
/*  Changed because the following won't compile on non-ANSI compilers */
/*              char xlat[] = "aeoeueAeOeUess>><<";
*/
                xlat[0] = 'a';  xlat[1] = 'e';
                xlat[2] = 'o';  xlat[3] = 'e';
                xlat[4] = 'u';  xlat[5] = 'e';
                xlat[6] = 'A';  xlat[7] = 'e';
                xlat[8] = 'O';  xlat[9] = 'e';
                xlat[10] = 'U'; xlat[11] = 'e';
                xlat[12] = 's'; xlat[13] = 's';
                xlat[14] = '>'; xlat[15] = '>';
                xlat[16] = '<'; xlat[17] = '<';

                xlat_buffer[0] = xlat[((c - 155) * 2) + 0];
                xlat_buffer[1] = xlat[((c - 155) * 2) + 1];
                xlat_buffer[2] = '\0';
            }
        }

        /* Substitute translated characters */

        for (i = 0; xlat_buffer[i] != '\0'; i++)
            write_char ((unsigned char) xlat_buffer[i]);

    }

}/* write_zchar */


/*
 * write_char
 *
 * High level character output routine. The write_char routine is slightly
 * complicated by the fact that the output can be limited by a fixed character
 * count, as well as, filling up the buffer.
 *
 */

#ifdef __STDC__
void write_char (int c)
#else
void write_char (c)
int c;
#endif
{    
    char *cp;
    int right_len;

    /* Only do if text formatting is turned on */

    if (formatting == ON && screen_window == TEXT_WINDOW) {

        /* Check to see if we have reached the right margin or exhausted our
           buffer space. This is complicated because not all printable attributes
           can be placed in the output buffer. This means that the actual
           number of displayed characters must be maintained separately. */

        if (fit_line (line, line_pos, screen_cols - right_margin) == 0 || char_count < 1) {

            /* Null terminate the line */

            line[line_pos] = '\0';

            /* If the next character is a space then no wrap is neccessary */

            if (c == ' ') {
                new_line ();
                c = '\0';
            } else {

                /* Wrap the line. First find the last space */

                cp = strrchr (line, ' ');

                /* If no spaces in the lines then cannot do wrap */

                if (cp == NULL) {

                    /* Output the buffer and a new line */

                    new_line ();

                } else {

                    /* Terminate the line at the last space */

                    *cp++ = '\0';

                    /* Calculate the text length after the last space */

                    right_len = &line[line_pos] - cp;

                    /* Output the buffer and a new line */

                    new_line ();

                    /* If any text to wrap then move it to the start of the line */

                    if (right_len > 0) {
                        memmove (line, cp, right_len);
                        line_pos = right_len;
                    }
                }
            }
        }

        /* Put the character into the buffer and count it.
           Decrement line width if the character is visible */

        if (c) {
            line[line_pos++] = (char) c;
            if (isprint (c))
                char_count--;
        }

    } else if (redirecting == ON) {

        /* If redirect is on then write the character to the status line for V1 to V3
           games or into the writeable data area for V4+ games */

        if (h_type < V4)
            status_line[status_pos++] = (char) c | 0x80;    // Always inverted
        else {
            set_byte (story_pos++, c);
            story_count++;
        }
    } else {

        /* No formatting or output redirection, so just output the character */

        //script_char (c);

        output_char (c);

    }

}/* write_char */

//#endif

/*
 * set_video_attribute
 *
 * Set a video attribute. Write the video mode, from 0 to 8, incremented.
 * This is so the output routines don't confuse video attribute 0 as the
 * end of the string.
 *
 */

#ifdef __STDC__
void set_video_attribute (zword_t mode)
#else
void set_video_attribute (mode)
zword_t mode;
#endif
{

    if ((int) mode <= MAX_ATTRIBUTE)
        write_char ((char) ++mode);

}/* set_video_attribute */

/*
 * write_string
 *
 * Output a string
 *
 */

#ifdef __STDC__
void write_string (const char *s)
#else
void write_string (s)
const char *s;
#endif
{

    while (pgm_read_byte(s))
        write_zchar (pgm_read_byte(s++));

}/* write_string */

/*
 * flush_buffer
 *
 * Send output buffer to the screen.
 *
 */

#ifdef __STDC__
void flush_buffer (int flag)
#else
void flush_buffer (flag)
int flag;
#endif
{
#if 1
    /* Terminate the line */

    line[line_pos] = '\0';

    /* Send the line buffer to the printer */

    //script_string (line);

    /* Send the line buffer to the screen */

    output_string (line);

    /* Reset the character count only if a carriage return is expected */

    if (flag == TRUE)
        char_count = screen_cols - right_margin;

    /* Reset the buffer pointer */

    line_pos = 0;
#endif

}/* flush_buffer */

/*
 * set_format_mode
 *
 * Set the format mode flag. Formatting disables writing into the output buffer.
 *
 */

#ifdef __STDC__
void set_format_mode (zword_t flag)
#else
void set_format_mode (flag)
zword_t flag;
#endif
{
#if 1
    /* Flush any current output */

    flush_buffer (FALSE);

    /* Set formatting depending on the flag */

    if (flag)
        formatting = ON;
    else
        formatting = OFF;
#endif
}/* set_format_mode */

/*
 * set_print_modes
 *
 * Set various printing modes. These can be: disabling output, scripting and
 * redirecting output. Redirection is peculiar. I use it to format the status
 * line for V1 to V3 games, otherwise it wasn't used. V4 games format the status line
 * themselves in an internal buffer in the writeable data area. To use the normal
 * text decoding routines they have to redirect output to the writeable data
 * area. This is done by passing in a buffer pointer. The first word of the
 * buffer will receive the number of characters written since the output was
 * redirected. The remainder of the buffer will contain the redirected text.
 *
 */

#ifdef __STDC__
void set_print_modes (zword_t type, zword_t option)
#else
void set_print_modes (type, option)
zword_t type;
zword_t option;
#endif
{
    
    if ((short) type == 1) {
        
        /* Turn on text output */
        
        outputting = ON;
        
    } else if ((short) type == 2) {
        
        /* Turn on scripting */
        
        //open_script ();
        
    } else if ((short) type == 3) {
        
        /* Turn on output redirection */
        
        /* Disable text formatting during redirection */
        
        saved_formatting = formatting;
        formatting = OFF;
        
        /* Enable text redirection */
        
        redirecting = ON;
        
        /* Set up the redirection pointers */
        
        if (h_type < V4)
            status_pos = 0;
            else {
                story_count = 0;
                story_buffer = option;
                story_pos = option + 2;
            }
        
    } else if ((short) type == 4) {
        
        /* Turn on input recording */
        
        //open_record ();
        
    } else if ((short) type == -1) {
        
        /* Turn off text output */
        
        outputting = OFF;
        
    } else if ((short) type == -2) {
        
        /* Turn off scripting */
        
        //close_script ();
        
    } else if ((short) type == -3) {
        
        /* Turn off output redirection */
        
        if (redirecting == ON) {
            
            /* Restore the format mode and turn off redirection */
            
            formatting = saved_formatting;
            redirecting = OFF;
            
            /* Terminate the redirection buffer and store the count of character
             in the buffer into the first word of the buffer */
            
            if (h_type > V3)
                set_word (story_buffer, story_count);
                
                }
        
    } else if ((short) type == -4) {
        
        /* Turn off input recording */
        
        //close_record ();
        
    }
    
}/* set_print_modes */

/*
 * print_character
 *
 * Write a character.
 *
 */

#ifdef __STDC__
void print_character (zword_t c)
#else
void print_character (c)
zword_t c;
#endif
{
    write_zchar ((char) c);
}/* print_character */

/*
 * print_number
 *
 * Write a signed number.
 *
 */

#if 0
#ifdef __STDC__
void print_number (zword_t num)
#else
void print_number (num)
zword_t num;
#endif
{
    int i, count;
    char buffer[10];

    i = (short) num;
    sprintf (buffer, "%d", i);
    count = strlen (buffer);
    for (i = 0; i < count; i++)
        write_char (buffer[i]);

}/* print_number */

#endif

/*
 * print_address
 *
 * Print using a packed address. Packed addresses are used to save space and
 * reference addresses outside of the data region.
 *
 */

#ifdef __STDC__
void print_address (zword_t packed_address)
#else
void print_address (packed_address)
zword_t packed_address;
#endif
{
    unsigned long address;

    /* Convert packed address to real address */

    address = (unsigned long) packed_address * story_scaler;

    /* Decode and output text at address */

    decode_text (&address);

}/* print_address */

/*
 * print_offset
 *
 * Print using a real address. Real addresses are just offsets into the
 * data region.
 *
 */

#ifdef __STDC__
void print_offset (zword_t offset)
#else
void print_offset (offset)
zword_t offset;
#endif
{
    unsigned long address;

    address = offset;

    /* Decode and output text at address */

    decode_text (&address);

}/* print_offset */

/*
 * print_object
 *
 * Print an object description. Object descriptions are stored as ASCIC
 * strings at the front of the property list for the object.
 *
 */

#ifdef __STDC__
void print_object (zword_t obj)
#else
void print_object (obj)
zword_t obj;
#endif
{
    zword_t offset;
    unsigned long address;

    /* Check for NULL object */

    if (obj == 0)
        return;

    /* Calculate address of property list */

    offset = get_object_address (obj);
    offset += (h_type < V4) ? O3_PROPERTY_OFFSET : O4_PROPERTY_OFFSET;

    /* Read the property list address and skip the count byte */

    address = (unsigned long) get_word (offset) + 1;

    /* Decode and output text at address */

    decode_text (&address);

}/* print_object */

/*
 * print_literal
 *
 * Print the string embedded in the instruction stream at this point. All
 * strings that do not need to be referenced by address are embedded in the
 * instruction stream. All strings that can be refered to by address are placed
 * at the end of the code region and referenced by packed address.
 *
 */

#ifdef __STDC__
void print_literal (void)
#else
void print_literal ()
#endif
{

    /* Decode and output text at PC */

    decode_text (&pc);

}/* print_literal */

/*
 * println_return
 *
 * Print a string embedded in the instruction stream as with print_literal,
 * except flush the output buffer and write a new line. After this return from
 * the current subroutine with a status of true.
 *
 */

#ifdef __STDC__
void println_return (void)
#else
void println_return ()
#endif
{

    print_literal ();
    new_line ();
    ret (TRUE);

}/* println_return */

/*
 * new_line
 *
 * Simply flush the current contents of the output buffer followed by a new
 * line.
 *
 */

#if 1

#ifdef __STDC__
void new_line (void)
#else
void new_line ()
#endif
{

    /* Only flush buffer if story redirect is off */

    if (redirecting == OFF) {
        flush_buffer (TRUE);
        //script_new_line ();
        output_new_line ();
    } else
        write_char ('\r');

}/* new_line */

#endif

/*
 * print_time
 *
 * Print the time as HH:MM [am|pm]. This is a bit language dependent and can
 * quite easily be changed. If you change the size of the time string output
 * then adjust the status line position in display_status_line.
 *
 */

prog_char s_am[] PROGMEM = " am";
prog_char s_pm[] PROGMEM = " pm";

#ifdef __STDC__
void print_time (int hours, int minutes)
#else
void print_time (hours, minutes)
int hours;
int minutes;
#endif
{
    int pm_indicator;

    /* Remember if time is pm */

    pm_indicator = (hours < 12) ? OFF : ON;

    /* Convert 24 hour clock to 12 hour clock */

    hours %= 12;
    if (hours == 0)
        hours = 12;

    /* Write hour right justified */

    if (hours < 10)
        write_char (' ');
    print_number (hours);

    /* Write hours/minutes separator */

    write_char (':');

    /* Write minutes zero filled */

    if (minutes < 10)
        write_char ('0');
    print_number (minutes);

    /* Write the am or pm string */

    if (pm_indicator == ON)
        write_string (s_pm);
    else
        write_string (s_am);

}/* print_time */

/*
 * encode
 *
 * Convert text to packed text.
 *
 */

#ifdef __STDC__
void encode (zword_t word_addr, zword_t word_length, zword_t word_offset, zword_t dest_addr)
#else
void encode (word_addr, word_length, word_offset, dest_addr)
zword_t word_addr;
zword_t word_length;
zword_t word_offset;
zword_t dest_addr;
#endif
{
    short word[3];
    int i;

    /* Encode the word */

    encode_text (word_length, word_addr + word_offset, word);

    /* Move the encoded word, byte swapped, into the destination buffer */

    for (i = 0; i < 3; i++, dest_addr += 2)
        set_word (dest_addr, word[i]);

}/* encode */
