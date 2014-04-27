/*
 * jzip.c       2.0.1g
 *
 * Z code interpreter main routine. Plays Infocom type 1, 2, 3, 4 and 5 games,
 * and Inform type 1-8 games.
 *
 * Usage: jzip [options] story-file-name
 *
 * options are:
 *
 *    -l n - number of lines in display
 *    -c n - number of columns in display
 *    -r n - right margin (default = 0)
 *    -t n - top margin (default = 0)
 *    -v   - version information
 *    -T   - set the tandy bit
 *
 * This is a no bells and whistles Infocom interpreter for type 1 to 5 games.
 * It will automatically detect which type of game you want to play. It should
 * support all type 1 to 5 features and is based loosely on the MS-DOS version
 * with enhancements to aid portability. Read the readme.txt file for
 * information on building this program on your favourite operating system.
 * Please mail me, at the address below, if you find bugs in the code.
 *
 * Special thanks to David Doherty and Olaf Barthel for testing this program
 * and providing invaluable help and code to aid its portability.
 *
 * Mark Howell 10-Mar-93 V2.0 howell_ma@movies.enet.dec.com
 * John Holder 22-Nov-95 V2.0.1g jholder@nmsu.edu
 *
 * Disclaimer:
 *      You are expressly forbidden to use this program if in so doing 
 *      you violate the copyright notice supplied with any original 
 *      Infocom or Inform game.  You may not charge any fee for this 
 *      program beyond a small distribution fee.  This program may
 *      be distributed freely provided this information is 
 *      included in the distribution, and that all files in the
 *      distribution are unaltered.  If you have problems with this
 *      interpreter and/or fix bugs in it, please notify John Holder
 *      (jholder@nmsu.edu) and he will add your fix and your name
 *      to the official JZIP distribution.
 */

#include "ztypes.h"

extern int GLOBALVER;

/*
 * configure
 *
 * Initialise global and type specific variables.
 *
 */

void configure (zbyte_t min_version, zbyte_t max_version)
{
    h_type = get_byte (H_TYPE);
    //GLOBALVER = h_type;

    if (h_type < min_version || h_type > max_version || (get_byte (H_CONFIG) & CONFIG_BYTE_SWAPPED))
        fatal (WRONG_GAME_OR_VERSION);
    if (h_type == V6 || h_type == V7)
        fatal (UNSUPPORTED_ZCODE_VERSION);

    if (h_type < V4) {
	story_scaler = 2;
	story_shift = 1;
	property_mask = P3_MAX_PROPERTIES - 1;
	property_size_mask = 0xe0;
    } else {
	story_scaler = 4;
	story_shift = 2;
	property_mask = P4_MAX_PROPERTIES - 1;
	property_size_mask = 0x3f;
    }

    /* 28-June-1995: Patched for Z-Code version 8, John Holder */  
    if (h_type == V8) { h_type=V5; story_scaler = 8; }

    h_config = get_byte (H_CONFIG);
    //h_version = get_word (H_VERSION);
    //h_data_size = get_word (H_DATA_SIZE);
    h_start_pc = get_word (H_START_PC);
    h_words_offset = get_word (H_WORDS_OFFSET);
    h_objects_offset = get_word (H_OBJECTS_OFFSET);
    h_globals_offset = get_word (H_GLOBALS_OFFSET);
    //h_restart_size = get_word (H_RESTART_SIZE);
    //h_flags = get_word (H_FLAGS);
    h_synonyms_offset = get_word (H_SYNONYMS_OFFSET);
    //h_file_size = get_word (H_FILE_SIZE);
    //if (h_file_size == 0)
    //    h_file_size = get_story_size ();
    //h_checksum = get_word (H_CHECKSUM);
    h_alternate_alphabet_offset = get_word (H_ALTERNATE_ALPHABET_OFFSET);
}/* configure */
