/*
 * input.c
 *
 * Input routines
 *
 */

#include "ztypes.h"

/* Statically defined word separator list */

//static const char *separators = " \t\n\f.,?";
PROGMEM const char s_separators[] = " \t\n\f.,?";

static zword_t dictionary_offset = 0;
static short dictionary_size = 0;
static unsigned int entry_size = 0; // 8 TODO

static void tokenise_line (zword_t, zword_t, zword_t, zword_t);
static zword_t next_token (zword_t s, zword_t *token, int *length, const char *punctuation);
static zword_t find_word (int, zword_t, long);

/*
 * read_character
 *
 * Read one character with optional timeout
 *
 *    argv[0] = # of characters to read (only 1 supported currently)
 *    argv[1] = timeout value in seconds (optional)
 *    argv[2] = timeout action routine (optional)
 *
 */

#ifdef __STDC__
void read_character (int argc, zword_t *argv)
#else
void read_character (argc, argv)
int argc;
zword_t *argv;
#endif
{
    int c;
    zword_t arg_list[2];

    /* Supply default parameters */

    if (argc < 3)
	argv[2] = 0;
    if (argc < 2)
	argv[1] = 0;

    /* Flush any buffered output before read */

    flush_buffer (FALSE);

    /* If more than one characters was asked for then fail the call */

    if (argv[0] != 1)

	c = 0;

    else {

	//if ((c = playback_key ()) == -1) {

	    /* Setup the timeout routine argument list */

	    arg_list[0] = argv[2];
	    arg_list[1] = argv[1]/10;

	    /* Read a character with a timeout. If the input timed out then
	       call the timeout action routine. If the return status from the
	       timeout routine was 0 then try to read a character again */

	    do {
		c = input_character ((int) argv[1]);
	    } while (c == -1 && call (2, arg_list, ASYNC) == 0);

	    /* Fail call if input timed out */

	    if (c == -1)
		c = 0;
	    //else
		//record_key (c);
	//}
    }

    store_operand (c);

}/* read_character */

/*
 * read_line
 *
 * Read a line of input with optional timeout.
 *
 *    argv[0] = character buffer address
 *    argv[1] = token buffer address
 *    argv[2] = timeout value in seconds (optional)
 *    argv[3] = timeout action routine (optional)
 *
 */

#ifdef __STDC__
void read_line (int argc, zword_t *argv)
#else
void read_line (argc, argv)
int argc;
zword_t *argv;
#endif
{
    int terminator;

    /* Supply default parameters */

    if (argc < 4)
	argv[3] = 0;
    if (argc < 3)
	argv[2] = 0;
    if (argc < 2)
	argv[1] = 0;

    /* Refresh status line */

    if (h_type < V4)
        display_status_line ();
                    
    /* Flush any buffered output before read */
        
    flush_buffer (TRUE);
    lines_written = 0;
        
    /* Read the line then script and record it */

    terminator = get_line( argv[0], argv[2], argv[3] );
        
    /* Tokenise the line, if a token buffer is present */

    if (argv[1])
        tokenise_line (argv[0], argv[1], h_words_offset, 0);

    /* Return the line terminator */

    if (h_type > V4)
        store_operand ((zword_t) terminator);

}/* read_line */

/*
 * get_line
 *
 * Read a line of input and lower case it.
 *
 */

int get_line (zword_t cbuf, zword_t timeout, zword_t action_routine)
{
    unsigned long a = cbuf;
    int buflen, read_size, status, c;
    zword_t arg_list[2];
    

    /* Set maximum buffer size to width of screen minus any
       right margin and 1 character for a terminating NULL */

    buflen = read_data_byte(&a);
    if (buflen > (TEXT_COLS-2))
        buflen = TEXT_COLS-2;

    /* Set read size and start of read buffer. The buffer may already be
       primed with some text in V5 games. The Z-code will have already
       displayed the text so we don't have to do that */

    read_size = h_type > V4 ? read_data_byte(&a) : 0;

	/* Setup the timeout routine argument list */

	arg_list[0] = action_routine;
	arg_list[1] = timeout/10;

	/* Read a line with a timeout. If the input timed out then
	   call the timeout action routine. If the return status from the
	   timeout routine was 0 then try to read the line again */

	do {
	    c = input_line (buflen, a, timeout, &read_size);
	    status = 0;
	} while (c == -1 && (status = call (2, arg_list, ASYNC)) == 0);

	/* Throw away any input if timeout returns success */

	if (status)
	    read_size = 0;

    /* Zero terminate line */
    a = cbuf+1;
    if ( h_type > V4 )
        set_byte(a, read_size++);   // space:cnt:input:0
    set_byte(a + read_size, 0);     // space:input:0

    return (c);

}/* get_line */

/*
 * tokenise_line
 *
 * Convert a typed input line into tokens. The token buffer needs some
 * additional explanation. The first byte is the maximum number of tokens
 * allowed. The second byte is set to the actual number of token read. Each
 * token is composed of 3 fields. The first (word) field contains the word
 * offset in the dictionary, the second (byte) field contains the token length,
 * and the third (byte) field contains the start offset of the token in the
 * character buffer.
 *
 */

#ifdef __STDC__
static void tokenise_line (zword_t char_buf, zword_t token_buf, zword_t dictionary, zword_t flag)
#else
static void tokenise_line (char_buf, token_buf, dictionary, flag)
zword_t char_buf;
zword_t token_buf;
zword_t dictionary;
zword_t flag;
#endif
{
    int i, count, words, token_length;
    long word_index, chop = 0;
    zword_t cp,tp,token = 0;
    char punctuation[16];
    zword_t word;

    /* Initialise word count and pointers */

    words = 0;
    cp = (h_type > V4) ? char_buf + 2 : char_buf + 1;
    tp = token_buf + 2;

    /* Initialise dictionary */

    count = get_byte (dictionary++);
    for (i = 0; i < count; i++)
        punctuation[i] = get_byte (dictionary++);
    punctuation[i] = '\0';
    entry_size = get_byte (dictionary++);
    dictionary_size = (short) get_word (dictionary);
    dictionary_offset = dictionary + 2;

    /* Calculate the binary chop start position */

    if (dictionary_size > 0) {
        word_index = dictionary_size / 2;
        chop = 1;
        do
            chop *= 2;
        while (word_index /= 2);
    }

    /* Tokenise the line */

    zbyte_t space = get_byte(token_buf);
    do {

	/* Skip to next token */

	cp = next_token (cp, &token, &token_length, punctuation);
    if (token_length) {

	    /* If still space in token buffer then store word */

	    if (words <= space) {

            /* Get the word offset from the dictionary */

            word = find_word (token_length, token, chop);

            /* Store the dictionary offset, token length and offset */

            if (word || flag == 0) {
                set_byte(tp,(char) (word >> 8));
                set_byte(tp+1,(char) (word & 0xff));
            }
            set_byte(tp+2,(char) token_length);
            set_byte(tp+3,(char) (token - char_buf));

            /* Step to next token position and count the word */

            tp += 4;
            words++;
	    } else {

            /* Moan if token buffer space exhausted */
            //output_string ("Too many words typed, discarding: ");
            //output_line (token);
	    }
      }
    } while (token_length);

    /* Store word count */

    set_byte(token_buf+1,(char) words);

}/* tokenise_line */

/*
 * next_token
 *
 * Find next token in a string. The token (word) is delimited by a statically
 * defined and a game specific set of word separators. The game specific set
 * of separators look like real word separators, but the parser wants to know
 * about them. An example would be: 'grue, take the axe. go north'. The
 * parser wants to know about the comma and the period so that it can correctly
 * parse the line. The 'interesting' word separators normally appear at the
 * start of the dictionary, and are also put in a separate list in the game
 * file.
 *
 */

static zword_t next_token (zword_t s, zword_t *token, int *length, const char *punctuation)
{
    int i;

    /* Set the token length to zero */

    *length = 0;

    /* Step through the string looking for separators */

    zbyte_t c;
    for (; (c = get_byte(s)); s++) {

	/* Look for game specific word separators first */

	for (i = 0; punctuation[i] && c != punctuation[i]; i++)
	    ;

	/* If a separator is found then return the information */

	if (punctuation[i]) {

	    /* If length has been set then just return the word position */

	    if (*length)
		return (s);
	    else {

		/* End of word, so set length, token pointer and return string */

		(*length)++;
		*token = s;
		return (++s);
	    }
	}

	/* Look for statically defined separators last */

    char separators[8];
    strcpy_P(separators,s_separators);
	for (i = 0; separators[i] && c != separators[i]; i++)
	    ;

	/* If a separator is found then return the information */

	if (separators[i]) {

	    /* If length has been set then just return the word position */

	    if (*length)
		return (++s);
	} else {

	    /* If first token character then remember its position */

	    if (*length == 0)
		*token = s;
	    (*length)++;
	}
    }

    return (s);

}/* next_token */

/*
 * find_word
 *
 * Search the dictionary for a word. Just encode the word and binary chop the
 * dictionary looking for it.
 *
 */

static zword_t find_word (int len, zword_t cp, long chop)
{
    short int word[3];
    long word_index, offset, status;

    /* Don't look up the word if there are no dictionary entries */

    if (dictionary_size == 0)
        return (0);

    /* Encode target word */

    encode_text (len, cp, word);

    /* Do a binary chop search on the main dictionary, otherwise do
       a linear search */

    word_index = chop - 1;

    if (dictionary_size > 0) {

	/* Binary chop until the word is found */

	while (chop) {

	    chop /= 2;

	    /* Calculate dictionary offset */

	    if (word_index > (dictionary_size - 1))
		word_index = dictionary_size - 1;

	    offset = dictionary_offset + (word_index * entry_size);

	    /* If word matches then return dictionary offset */

	    if ((status = word[0] - (short) get_word (offset + 0)) == 0 &&
		(status = word[1] - (short) get_word (offset + 2)) == 0 &&
		(h_type < V4 ||
		(status = word[2] - (short) get_word (offset + 4)) == 0))
		return ((zword_t) offset);

	    /* Set next position depending on direction of overshoot */

	    if (status > 0) {
		word_index += chop;

		/* Deal with end of dictionary case */

		if (word_index >= (int) dictionary_size)
		    word_index = dictionary_size - 1;
	    } else {
		word_index -= chop;

		/* Deal with start of dictionary case */

		if (word_index < 0)
		    word_index = 0;
	    }
	}
    } else {

	for (word_index = 0; word_index < -dictionary_size; word_index++) {

	    /* Calculate dictionary offset */

	    offset = dictionary_offset + (word_index * entry_size);

	    /* If word matches then return dictionary offset */

	    if ((status = word[0] - (short) get_word (offset + 0)) == 0 &&
		(status = word[1] - (short) get_word (offset + 2)) == 0 &&
		(h_type < V4 ||
		(status = word[2] - (short) get_word (offset + 4)) == 0))
		return ((zword_t) offset);
	}
    }

    return (0);

}/* find_word */

/*
 * tokenise
 *
 *    argv[0] = character buffer address
 *    argv[1] = token buffer address
 *    argv[2] = alternate vocabulary table
 *    argv[3] = ignore unknown words flag
 *
 */

#ifdef __STDC__
void tokenise (int argc, zword_t *argv)
#else
void tokenise (argc, argv)
int argc;
zword_t *argv;
#endif
{

    /* Supply default parameters */

    if (argc < 4)
	argv[3] = 0;
    if (argc < 3)
	argv[2] = h_words_offset;

    /* Convert the line to tokens */

    tokenise_line (argv[0], argv[1], argv[2], argv[3]);

}/* tokenise */
