/*
 * extern.c
 *
 * Global data.
 *
 */

#include "ztypes.h"

//int GLOBALVER;

/* Game header data */

zbyte_t h_type = 0;
zbyte_t h_config = 0;
//zword_t h_version = 0;
//zword_t h_data_size = 0;
zword_t h_start_pc = 0;
zword_t h_words_offset = 0;
zword_t h_objects_offset = 0;
zword_t h_globals_offset = 0;
//zword_t h_restart_size = 0;
//zword_t h_flags = 0;
zword_t h_synonyms_offset = 0;
//zword_t h_file_size = 0;
zword_t h_checksum = 0;
//zbyte_t h_interpreter = INTERP_MSDOS;
//zbyte_t h_interpreter_version = 'B'; /* Interpreter version 2 */
zword_t h_alternate_alphabet_offset = 0;

/* Game version specific data */

uint8_t story_scaler = 0;
uint8_t story_shift = 0;
uint8_t property_mask = 0;
uint8_t property_size_mask = 0;

/* Stack and PC data */

zword_t sp = STACK_SIZE;
zword_t fp = STACK_SIZE - 1;
unsigned long pc = 0;
uint8_t interpreter_state = RUN;
int interpreter_status = 0;

/* Current window data */

uint8_t screen_window = TEXT_WINDOW;
uint8_t status_size = 0;
uint8_t status_active = 0;
int8_t char_count = 0; // signed!

uint8_t redirecting = 0;
uint8_t state_flags = 0;

//int interp_initialized = 0;
