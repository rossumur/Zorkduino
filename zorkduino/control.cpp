/*
 * control.c
 *
 * Functions that alter the flow of control.
 *
 */

#include "ztypes.h"

const prog_char v1_lookup_table[] PROGMEM = {
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    " 0123456789.,!?_#'\"/\\<-:()"
};

const prog_char v3_lookup_table[] PROGMEM = {
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    " \n0123456789.,!?_#'\"/\\-:()"
};

char lookup_table(uint8_t i, uint8_t c)
{
    i = i*26 + c;
    if (h_alternate_alphabet_offset)
        return get_byte(h_alternate_alphabet_offset + i);
    if (h_type >= V3)
        return pgm_read_byte(v3_lookup_table+i);
    return pgm_read_byte(v1_lookup_table+i);
}

/*
 * check_argument
 *
 * Jump if argument is present.
 *
 */

#ifdef __STDC__
void check_argument (zword_t argc)
#else
void check_argument (argc)
zword_t argc;
#endif
{

    conditional_jump (argc <= (zword_t) (STACK(fp + 1) & ARGS_MASK));

}/* check_argument */

/*
 * call
 *
 * Call a subroutine. Save PC and FP then load new PC and initialise stack based
 * local arguments.
 *
 */

#ifdef __STDC__
int call (int argc, zword_t *argv, int type)
#else
int call (argc, argv, type)
int argc;
zword_t *argv;
int type;
#endif
{
    zword_t arg;
    int i = 1, args, status = 0;

    /* Convert calls to 0 as returning FALSE */

    if (argv[0] == 0) {
        if (type == FUNCTION)
            store_operand (FALSE);
        return (0);
    }

    /* Save current PC, FP and argument count on stack */
    PUSH(pc / PAGE_SIZE);
    PUSH(pc % PAGE_SIZE);
    PUSH(fp);
    PUSH((argc - 1) | type);
    
    /* Create FP for new subroutine and load new PC */

    fp = sp - 1;
    pc = (unsigned long) argv[0] * story_scaler;

    /* Read argument count and initialise local variables */

    args = (unsigned int) read_code_byte ();
    while (--args >= 0) {
        arg = (h_type > V4) ? 0 : read_code_word ();
        PUSH((--argc > 0) ? argv[i++] : arg);
    }

    /* If the call is asynchronous then call the interpreter directly.
       We will return back here when the corresponding return frame is
       encountered in the ret call. */

    if (type == ASYNC) {
        status = interpret ();
        interpreter_state = RUN;
        interpreter_status = 1;
    }

    return (status);

}/* call */

/*
 * ret
 *
 * Return from subroutine. Restore FP and PC from stack.
 *
 */

#ifdef __STDC__
void ret (zword_t value)
#else
void ret (value)
zword_t value;
#endif
{
    zword_t argc;

    /* Clean stack */

    sp = fp + 1;

    /* Restore argument count, FP and PC */

    argc = POP();
    fp = POP();
    pc = POP();
    pc += ((unsigned long) POP()) * PAGE_SIZE;

    /* If this was an async call then stop the interpreter and return
       the value from the async routine. This is slightly hacky using
       a global state variable, but ret can be called with conditional_jump
       which in turn can be called from all over the place, sigh. A
       better design would have all opcodes returning the status RUN, but
       this is too much work and makes the interpreter loop look ugly */

    if ((argc & TYPE_MASK) == ASYNC) {

        interpreter_state = STOP;
        interpreter_status = (int) value;

    } else {

        /* Return subroutine value for function call only */

        if ((argc & TYPE_MASK) == FUNCTION)
            store_operand (value);

    }

}/* ret */

/*
 * jump
 *
 * Unconditional jump. Jump is PC relative.
 *
 */

#ifdef __STDC__
void jump (zword_t offset)
#else
void jump (offset)
zword_t offset;
#endif
{

    pc = (unsigned long) (pc + (short) offset - 2);

}/* jump */

/*
 * restart
 *
 * Restart game by initialising environment and reloading start PC.
 *
 */

#ifdef __STDC__
void restart (void)
#else
void restart ()
#endif
{
    //unsigned int scripting_flag;

    /* Reset output buffer */

    flush_buffer (TRUE);

    /* Reset text control flags */

    formatting = ON;
    outputting = ON;
    redirecting = OFF;
    //scripting_disable = OFF;

    /* Randomise */

  // TODO BUGBUG TODO
   // srand ((unsigned int) time (NULL));

    /* Remember scripting state */

   // scripting_flag = get_word (H_FLAGS) & SCRIPTING_FLAG;

    /* Load restart size and reload writeable data area */

    //restart_size = (h_restart_size / PAGE_SIZE) + 1;
    //for (i = 0; i < restart_size; i++)
    //  read_page (i, &datap[i * PAGE_SIZE]);

    /* Restart the screen */

    set_status_size (0);
    set_colours(1,1); /* set default colors, added by JDH 8/6/95 */
    set_attribute (NORMAL);
    erase_window (SCREEN);

    restart_screen ();

    /* Reset the interpreter state */

    //if (scripting_flag)
    //    set_word (H_FLAGS, (get_word (H_FLAGS) | SCRIPTING_FLAG));

    set_byte (H_INTERPRETER, INTERP_MSDOS);
    set_byte (H_INTERPRETER_VERSION, 'B');
    set_byte (H_SCREEN_ROWS, TEXT_ROWS); /* Screen dimension in characters */
    set_byte (H_SCREEN_COLUMNS, TEXT_COLS); //TEXT_COLS);

    zword_t h_checksum = get_word (H_CHECKSUM);
    if (h_checksum == 5803) {
        set_byte (H_SCREEN_COLUMNS, 80);    // TRINITY hack TODO BUGBUG
    }

    set_byte (H_SCREEN_LEFT, 0); /* Screen dimension in smallest addressable units, ie. pixels */
    set_byte (H_SCREEN_RIGHT, TEXT_COLS); //TEXT_COLS);
    set_byte (H_SCREEN_TOP, 0);
    set_byte (H_SCREEN_BOTTOM, TEXT_ROWS);

    set_byte (H_MAX_CHAR_WIDTH, 1); /* Size of a character in screen units */
    set_byte (H_MAX_CHAR_HEIGHT, 1);

    /* Initialise status region */

    if (h_type < V4) {
       set_status_size(0);
       blank_status_line();
    }
    
    /* Load start PC, SP and FP */

    pc = h_start_pc;
    sp = STACK_SIZE;
    fp = STACK_SIZE - 1;
}/* restart */

/*
 * get_fp
 *
 * Return the value of the frame pointer (FP) for later use with unwind.
 * Before V5 games this was a simple pop.
 *
 */

#ifdef __STDC__
void get_fp (void)
#else
void get_fp ()
#endif
{

    if (h_type > V4)
        store_operand (fp);
    else
        sp++;

}/* get_fp */

/*
 * unwind
 *
 * Remove one or more stack frames and return. Works like longjmp, see get_fp.
 *
 */

#ifdef __STDC__
void unwind (zword_t value, zword_t new_fp)
#else
void unwind (value, new_fp)
zword_t value;
zword_t new_fp;
#endif
{

    if (new_fp > fp)
        fatal (ILLEGAL_OPERATION);

    fp = new_fp;
    ret (value);

}/* unwind */
