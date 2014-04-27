/*
 * ztypes.h
 *
 * Any global stuff required by the C modules.
 *
 */

#if !defined(__ZTYPES_INCLUDED)
#define __ZTYPES_INCLUDED

/* don't define the following unless you have john.c and john.h! */
/*
 */

/* AIX likes to see this define... */
#if defined(AIX)
#define _POSIX_SOURCE
#define POSIX
#endif

/* for Turbo C & MSC */
#if defined(__MSDOS__)
#define MSDOS
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <execinfo.h>

#ifdef ARDUINO
#include <Arduino.h>
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#else
#define PROGMEM
#define prog_char char
#define pgm_read_byte(_x) *(_x)
#define strcpy_P strcpy
#endif

//================================================================================
//================================================================================

#define SAVE_SIZE           ((64+2)*1024L)
#define SAVE_SLOTS          10
#define STACK_REGION_OFFSET 0
#define GAME_REGION_OFFSET  (STACK_REGION_OFFSET + 2048L)
#define SAVE_REGION_OFFSET  (GAME_REGION_OFFSET + 256*1024L)
#define MEMORY_FILE_SIZE    (SAVE_REGION_OFFSET + SAVE_SLOTS*SAVE_SIZE)

#define TEXT_COLS 38
#define TEXT_ROWS 24
extern uint8_t _fdata[TEXT_ROWS*TEXT_COLS];

extern void zdInit();
extern void zdLoop();

#define ILLEGAL_OPERATION           1
#define BAD_FRAME_FOR_UNWIND        2
#define WRONG_GAME_OR_VERSION       3
#define UNSUPPORTED_ZCODE_VERSION   4
#define NO_SUCH_PROPERTY            5

//================================================================================
//================================================================================


#if defined(MSDOS)
#include <malloc.h>
#endif /* MSDOS */

/* Set Version of JZIP */

#define JZIPVER "JZIP V2.0.1g"
#define JZIPRELDATE "Tue, 5 Dec 1995"
extern unsigned char JTERP;

/* Configuration options */

#define DEFAULT_ROWS 24 /* Default screen height */
#define DEFAULT_COLS 80 /* Deafult screen width */

#define DEFAULT_RIGHT_MARGIN 0 /* # of characters in right margin */
#define DEFAULT_TOP_MARGIN 0   /* # of lines left on screen before [MORE] message */

/* Global defines */

/* number of bits in a byte.  needed by AIX!!! ;^) */
#ifndef NBBY
#define NBBY 8
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

#ifndef PATHNAME_MAX
#define PATHNAME_MAX 1024
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifdef unix

#define strchr(a, b) index (a, b)

#if defined (HAVE_BCOPY)
#define memmove(a, b, c) bcopy (b, a, c)
#else
#define memmove(a, b, c) memcpy ((a), (b), (c))
#endif

#define const

#endif /* unix */

/* Z types */

typedef unsigned char zbyte_t;  /* unsigned 1 byte quantity */
typedef unsigned short zword_t; /* unsigned 2 byte quantity */

/* Data file header format */

typedef struct zheader {
    zbyte_t type;
    zbyte_t config;
    zword_t version;
    zword_t data_size;
    zword_t start_pc;
    zword_t words_offset;
    zword_t objects_offset;
    zword_t globals_offset;
    zword_t restart_size;
    zword_t flags;
    zbyte_t release_date[6];
    zword_t synonyms_offset;
    zword_t file_size;
    zword_t checksum;
    zbyte_t interpreter;
    zbyte_t interpreter_version;
    zbyte_t screen_rows;
    zbyte_t screen_columns;
    zbyte_t screen_left;
    zbyte_t screen_right;
    zbyte_t screen_top;
    zbyte_t screen_bottom;
    zbyte_t max_char_width;
    zbyte_t max_char_height;
    zword_t filler1[3];
    zword_t function_keys_offset;
    zword_t filler2[2];
    zword_t alternate_alphabet_offset;
    zword_t mouse_position_offset;
    zword_t filler3[4];
} zheader_t;

#define H_TYPE 0
#define H_CONFIG 1

#define CONFIG_BYTE_SWAPPED 0x01 /* Game data is byte swapped          - V3/V4 */
#define CONFIG_COLOUR       0x01 /* Game supports colour               - V5+   */
#define CONFIG_TIME         0x02 /* Status line displays time          - V3    */
#define CONFIG_MAX_DATA     0x04 /* Data area should 64K if possible   - V4+   */
#define CONFIG_TANDY        0x08 /* Tandy licensed game                - V3    */
#define CONFIG_EMPHASIS     0x08 /* Interpreter supports text emphasis - V4+   */
#define CONFIG_NOSTATUSLINE 0x10 /* Interpreter cannot support a status line   */
#define CONFIG_WINDOWS      0x20 /* Interpreter supports split screen mode     */

#define H_VERSION 2
#define H_DATA_SIZE 4
#define H_START_PC 6
#define H_WORDS_OFFSET 8
#define H_OBJECTS_OFFSET 10
#define H_GLOBALS_OFFSET 12
#define H_RESTART_SIZE 14
#define H_FLAGS 16

#define SCRIPTING_FLAG 0x01
#define FIXED_FONT_FLAG 0x02
#define REFRESH_FLAG 0x04
#define GRAPHICS_FLAG 0x08
#define SOUND_FLAG 0x10          /* V4 */
#define UNDO_AVAILABLE_FLAG 0x10 /* V5 */
#define COLOUR_FLAG 0x40
#define NEW_SOUND_FLAG 0x80

#define H_RELEASE_DATE 18
#define H_SYNONYMS_OFFSET 24
#define H_FILE_SIZE 26
#define H_CHECKSUM 28
#define H_INTERPRETER 30

#define INTERP_GENERIC 0
#define INTERP_DEC_20 1
#define INTERP_APPLE_IIE 2
#define INTERP_MACINTOSH 3
#define INTERP_AMIGA 4
#define INTERP_ATARI_ST 5
#define INTERP_MSDOS 6
#define INTERP_CBM_128 7
#define INTERP_CBM_64 8
#define INTERP_APPLE_IIC 9
#define INTERP_APPLE_IIGS 10
#define INTERP_TANDY 11
#define INTERP_UNIX 12
#define INTERP_VMS 13

#define H_INTERPRETER_VERSION 31
#define H_SCREEN_ROWS 32
#define H_SCREEN_COLUMNS 33
#define H_SCREEN_LEFT 34
#define H_SCREEN_RIGHT 35
#define H_SCREEN_TOP 36
#define H_SCREEN_BOTTOM 37
#define H_MAX_CHAR_WIDTH 38
#define H_MAX_CHAR_HEIGHT 39
#define H_FILLER1 40

#define H_FUNCTION_KEYS_OFFSET 46
#define H_FILLER2 48

#define H_ALTERNATE_ALPHABET_OFFSET 52
#define H_MOUSE_POSITION_OFFSET 54
#define H_FILLER3 56

#define V1 1

#define V2 2

/* Version 3 object format */

#define V3 3

typedef struct zobjectv3 {
    zword_t attributes[2];
    zbyte_t parent;
    zbyte_t next;
    zbyte_t child;
    zword_t property_offset;
} zobjectv3_t;

#define O3_ATTRIBUTES 0
#define O3_PARENT 4
#define O3_NEXT 5
#define O3_CHILD 6
#define O3_PROPERTY_OFFSET 7

#define O3_SIZE 9

#define PARENT3(offset) (offset + O3_PARENT)
#define NEXT3(offset) (offset + O3_NEXT)
#define CHILD3(offset) (offset + O3_CHILD)

#define P3_MAX_PROPERTIES 0x20

/* Version 4 object format */

#define V4 4

typedef struct zobjectv4 {
    zword_t attributes[3];
    zword_t parent;
    zword_t next;
    zword_t child;
    zword_t property_offset;
} zobjectv4_t;

#define O4_ATTRIBUTES 0
#define O4_PARENT 6
#define O4_NEXT 8
#define O4_CHILD 10
#define O4_PROPERTY_OFFSET 12

#define O4_SIZE 14

#define PARENT4(offset) (offset + O4_PARENT)
#define NEXT4(offset) (offset + O4_NEXT)
#define CHILD4(offset) (offset + O4_CHILD)

#define P4_MAX_PROPERTIES 0x40

#define V5 5
#define V6 6
#define V7 7
#define V8 8

/* Interpreter states */

#define STOP 0
#define RUN 1

/* Call types */

#define FUNCTION 0x0000
#define PROCEDURE 0x0100
#define ASYNC 0x0200

#define ARGS_MASK 0x00ff
#define TYPE_MASK 0xff00

/* Local defines */

#define PAGE_SIZE 512
#define PAGE_MASK 511
#define PAGE_SHIFT 9

#define STACK_SIZE 1024

#define ON 1
#define OFF 0
#define RESET -1

#define SCREEN 255
#define TEXT_WINDOW 0
#define STATUS_WINDOW 1

#define MIN_ATTRIBUTE 0
#define NORMAL 0
#define REVERSE 1
#define BOLD 2
#define EMPHASIS 4
#define FIXED_FONT 8
#define MAX_ATTRIBUTE 8

#define TEXT_FONT 1
#define GRAPHICS_FONT 3

#define FOREGROUND 0
#define BACKGROUND 1

#define GAME_RESTORE 0
#define GAME_SAVE 1
#define GAME_SCRIPT 2
#define GAME_RECORD 3
#define GAME_PLAYBACK 4
#define UNDO_SAVE 5
#define UNDO_RESTORE 6

#define MAX_TEXT_SIZE 8

/* Data access */

zbyte_t get_byte(unsigned long offset);
zword_t get_word(unsigned long offset);
void set_byte(zword_t offset,zbyte_t value);
void set_word(zword_t offset,zword_t value);

/* External data */

extern int GLOBALVER;
extern zbyte_t h_type;
extern zbyte_t h_config;
extern zword_t h_version;
extern zword_t h_data_size;
extern zword_t h_start_pc;
extern zword_t h_words_offset;
extern zword_t h_objects_offset;
extern zword_t h_globals_offset;
extern zword_t h_restart_size;
extern zword_t h_flags;
extern zword_t h_synonyms_offset;
extern zword_t h_file_size;
extern zword_t h_checksum;
extern zbyte_t h_interpreter;
extern zbyte_t h_interpreter_version;
extern zword_t h_alternate_alphabet_offset;

extern uint8_t story_scaler;
extern uint8_t story_shift;
extern uint8_t property_mask;
extern uint8_t property_size_mask;

extern void PUSH(zword_t v);
extern zword_t POP();
extern zword_t STACK(zword_t i);
extern void STACK(zword_t i,zword_t v);

extern zword_t sp;
extern zword_t fp;
extern unsigned long pc;
extern uint8_t interpreter_state;
extern int interpreter_status;

//extern unsigned int data_size;
//extern zbyte_t *datap;
//extern zbyte_t *undo_datap;

extern uint8_t screen_window;
//extern int interp_initialized;

extern uint8_t formatting;
extern uint8_t outputting;
extern uint8_t redirecting;
extern uint8_t scripting;
extern uint8_t scripting_disable;
extern uint8_t recording;
extern uint8_t replaying;
//extern int font;

#define FORMATTING  1
#define OUTPUTTING  2
#define REDIRECTING 4
#define SCRIPTING   8
#define RECORDING   16
#define REPLAYING   32
#define STATUS_ACTIVE   64
extern uint8_t state_flags;


extern uint8_t status_active;
extern uint8_t status_size;
extern int8_t char_count;

#define right_margin    0
#define left_margin     0
#define top_margin      0
#define screen_cols     TEXT_COLS
#define screen_rows     TEXT_ROWS

extern uint8_t lines_written;
extern uint8_t status_pos;

extern char *line;
extern char *status_line;

//extern char lookup_table[3][26];
char lookup_table(uint8_t i, uint8_t c);

extern char monochrome;
extern int hist_buf_size;
extern char bigscreen;

/* Global routines */

/* control.c */

#ifdef __STDC__
void check_argument (zword_t);
int call (int, zword_t *, int);
void get_fp (void);
void jump (zword_t);
void restart (void);
void ret (zword_t);
void unwind (zword_t, zword_t);
#else
void check_argument ();
int call ();
void get_fp ();
void jump ();
void restart ();
void ret ();
void unwind ();
#endif

/* fileio.c */

#ifdef __STDC__
void close_record (void);
void close_script (void);
void close_story (void);
unsigned int get_story_size (void);
void open_playback (int);
void open_record (void);
void open_script (void);
int playback_key (void);
int playback_line (int, char *, int *);
void read_page (int, void *);
void record_key (int);
void record_line (const char *);
int restore (void);
int save (void);
void script_char (int);
void script_string (const char *);
void script_line (const char *);
void script_new_line (void);
void undo_restore (void);
void undo_save (void);
void verify (void);
#else
void close_record ();
void close_script ();
void close_story ();
unsigned int get_story_size ();
void open_playback ();
void open_record ();
void open_script ();
void open_story ();
int playback_key ();
int playback_line ();
void read_page ();
void record_key ();
void record_line ();
int restore ();
int save ();
void script_char ();
void script_string ();
void script_line ();
void script_new_line ();
void undo_restore ();
void undo_save ();
void verify ();
#endif

/* input.c */

#ifdef __STDC__
int get_line (zword_t, zword_t, zword_t);
void read_character (int, zword_t *);
void read_line (int, zword_t *);
void tokenise (int, zword_t *);
#else
int get_line ();
void read_character ();
void read_line ();
void tokenise ();
#endif

/* interpre.c */

#ifdef __STDC__
int interpret (void);
#else
int interpret ();
#endif

/* math.c */

#ifdef __STDC__
void add (zword_t, zword_t);
void and_ (zword_t, zword_t);
void arith_shift (zword_t, zword_t);
void compare_je (int, zword_t *);
void compare_jg (zword_t, zword_t);
void compare_jl (zword_t, zword_t);
void compare_zero (zword_t);
void divide (zword_t, zword_t);
void multiply (zword_t, zword_t);
void not_(zword_t);
void or_(zword_t, zword_t);
void zip_random (zword_t);
void remainder (zword_t, zword_t);
void shift (zword_t, zword_t);
void subtract (zword_t, zword_t);
void test (zword_t, zword_t);
#else
void add ();
void and ();
void arith_shift ();
void compare_je ();
void compare_jg ();
void compare_jl ();
void compare_zero ();
void divide ();
void multiply ();
void not ();
void or ();
void zip_random ();
void remainder ();
void shift ();
void subtract ();
void test ();
#endif

/* memory.c */
zbyte_t read_code_byte (void);
zbyte_t read_data_byte (unsigned long *);
zword_t read_code_word (void);
zword_t read_data_word (unsigned long *);

/* object.c */

#ifdef __STDC__
void clear_attr (zword_t, zword_t);
void compare_parent_object (zword_t, zword_t);
void insert_object (zword_t, zword_t);
void load_child_object (zword_t);
void load_next_object (zword_t);
void load_parent_object (zword_t);
void remove_object (zword_t);
void set_attr (zword_t, zword_t);
void test_attr (zword_t, zword_t);
zword_t get_object_address (zword_t);
#else
void clear_attr ();
void compare_parent_object ();
void insert_object ();
void load_child_object ();
void load_next_object ();
void load_parent_object ();
void remove_object ();
void set_attr ();
void test_attr ();
zword_t get_object_address ();
#endif

/* operand.c */

#ifdef __STDC__
void conditional_jump (int);
void store_operand (zword_t);
void store_variable (int, zword_t);
zword_t load_operand (int);
zword_t load_variable (int);
#else
void conditional_jump ();
void store_operand ();
void store_variable ();
zword_t load_operand ();
zword_t load_variable ();
#endif

/* osdepend.c */

#ifdef __STDC__
int codes_to_text (int, char *);
void fatal (uint8_t);
void file_cleanup (const char *, int);
int fit_line (const char *, int, int);
int get_file_name (char *, char *, int);
int print_status (int, char *[]);
void process_arguments (int, char *[]);
void set_colours (zword_t, zword_t);
void set_font (int);
void sound (int, zword_t *);
#else
int codes_to_text ();
void fatal ();
void file_cleanup ();
int fit_line ();
int get_file_name ();
int print_status ();
void process_arguments ();
void set_colours ();
void set_font ();
void sound ();
#endif

/* property.c */

#ifdef __STDC__
void load_byte (zword_t, zword_t);
void load_next_property (zword_t, zword_t);
void load_property (zword_t, zword_t);
void load_property_address (zword_t, zword_t);
void load_property_length (zword_t);
void load_word (zword_t, zword_t);
void move_data (zword_t, zword_t, zword_t);
void scan_data (int, zword_t *);
void store_byte (zword_t, zword_t, zword_t);
void store_property (zword_t, zword_t, zword_t);
void store_word (zword_t, zword_t, zword_t);
#else
void load_byte ();
void load_next_property ();
void load_property ();
void load_property_address ();
void load_property_length ();
void load_word ();
void move_data ();
void scan_data ();
void store_byte ();
void store_property ();
void store_word ();
#endif

/* screen.c */

#ifdef __STDC__
void blank_status_line (void);
void display_status_line (void);
void erase_line (zword_t);
void erase_window (zword_t);
void output_char (int);
void output_new_line (void);
void output_string (const char *);
void output_line (const char *);
void print_window (int, zword_t *);
void select_window (zword_t);
void set_cursor_position (zword_t, zword_t);
void set_colour_attribute (zword_t, zword_t);
void set_font_attribute (zword_t);
void set_status_size (zword_t);
#else
void blank_status_line ();
void display_status_line ();
void erase_line ();
void erase_window ();
void output_char ();
void output_new_line ();
void output_string ();
void output_line ();
void print_window ();
void select_window ();
void set_cursor_position ();
void set_colour_attribute ();
void set_font_attribute ();
void set_status_size ();
#endif

/* screenio.c */

#ifdef __STDC__
int input_character (int);
void clear_line (void);
void clear_screen (void);
void clear_status_window (void);
void clear_text_window (void);
void create_status_window (void);
void delete_status_window (void);
void display_char (int);
int fit_line (const char *, int, int);
void get_cursor_position (int *, int *);
void initialize_screen (void);
int input_line (int, unsigned long, int, int *);
void move_cursor (int, int);
int print_status (int, char *[]);
void reset_screen (void);
void restart_screen (void);
void restore_cursor_position (void);
void save_cursor_position (void);
void scroll_line (void);
void select_status_window (void);
void select_text_window (void);
void set_attribute (int);
#else
int input_character ();
void clear_line ();
void clear_screen ();
void clear_status_window ();
void clear_text_window ();
void create_status_window ();
void delete_status_window ();
void display_char ();
int fit_line ();
void get_cursor_position ();
void initialize_screen ();
int input_line ();
void move_cursor ();
int print_status ();
void reset_screen ();
void restart_screen ();
void restore_cursor_position ();
void save_cursor_position ();
void scroll_line ();
void select_status_window ();
void select_text_window ();
void set_attribute ();
#endif

/* text.c */

#ifdef __STDC__
void decode_text (unsigned long *);
void encode (zword_t, zword_t, zword_t, zword_t);
void encode_text (int, zword_t, short *);
void flush_buffer (int);
void new_line (void);
void print_address (zword_t);
void print_character (zword_t);
void print_literal (void);
void print_number (zword_t);
void print_object (zword_t);
void print_offset (zword_t);
void print_time (int, int);
void println_return (void);
void command_opcode (zword_t);
void set_format_mode (zword_t);
void set_print_modes (zword_t, zword_t);
void set_video_attribute (zword_t);
void write_char (int);
void write_string (const char *);
void write_zchar (int);
#else
void decode_text ();
void encode ();
void encode_text ();
void flush_buffer ();
void new_line ();
void print_address ();
void print_character ();
void print_literal ();
void print_number ();
void print_object ();
void print_offset ();
void print_time ();
void println_return ();
void command_opcode ();
void set_format_mode ();
void set_print_modes ();
void set_video_attribute ();
void write_char ();
void write_string ();
void write_zchar ();
#endif

/* variable.c */

#ifdef __STDC__
void decrement (zword_t);
void decrement_check (zword_t, zword_t);
void increment (zword_t);
void increment_check (zword_t, zword_t);
void load (zword_t);
void pop_var (zword_t);
void push_var (zword_t);
#else
void decrement ();
void decrement_check ();
void increment ();
void increment_check ();
void load ();
void pop_var ();
void push_var ();
#endif

#endif /* !defined(__ZTYPES_INCLUDED) */
