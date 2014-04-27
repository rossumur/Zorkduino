/*
 * operand.c
 *
 * Operand manipulation routines
 *
 */

#include "ztypes.h"

/*
 * load_operand
 *
 * Load an operand, either: a variable, popped from the stack or a literal.
 *
 */

#ifdef __STDC__
zword_t load_operand (int type)
#else
zword_t load_operand (type)
int type;
#endif
{
    zword_t operand;

    if (type) {

        /* Type 1: byte literal, or type 2: operand specifier */

        operand = (zword_t) read_code_byte ();
        if (type == 2) {

            /* If operand specifier non-zero then it's a variable, otherwise
               it's the top of the stack */

            if (operand)
                operand = load_variable (operand);
            else
                operand = POP();
        }
    } else

        /* Type 0: word literal */

        operand = read_code_word ();

    return (operand);

}/* load_operand */

/*
 * store_operand
 *
 * Store an operand, either as a variable pushed on the stack.
 *
 */

#ifdef __STDC__
void store_operand (zword_t operand)
#else
void store_operand (operand)
zword_t operand;
#endif
{
    zbyte_t specifier;

    /* Read operand specifier byte */

    specifier = read_code_byte ();

    /* If operand specifier non-zero then it's a variable, otherwise it's the
       pushed on the stack */

    if (specifier)
        store_variable (specifier, operand);
    else
        PUSH(operand);

}/* store_operand */

/*
 * load_variable
 *
 * Load a variable, either: a stack local variable, a global variable or the top
 * of the stack.
 *
 */

#ifdef __STDC__
zword_t load_variable (int number)
#else
zword_t load_variable (number)
int number;
#endif
{
    zword_t variable;

    if (number) {
        if (number < 16)

            /* number in range 1 - 15, it's a stack local variable */

            variable = STACK(fp - (number - 1));
        else

            /* number > 15, it's a global variable */

            variable = get_word (h_globals_offset + ((number - 16) * 2));
    } else

        /* number = 0, get from top of stack */

        variable = STACK(sp);

    return (variable);

}/* load_variable */

/*
 * store_variable
 *
 * Store a variable, either: a stack local variable, a global variable or the top
 * of the stack.
 *
 */

#ifdef __STDC__
void store_variable (int number, zword_t variable)
#else
void store_variable (number, variable)
int number;
zword_t variable;
#endif
{

    if (number) {
        if (number < 16)

            /* number in range 1 - 15, it's a stack local variable */

            STACK(fp - (number - 1),variable);
        else

            /* number > 15, it's a global variable */

            set_word (h_globals_offset + ((number - 16) * 2), variable);
    } else

        /* number = 0, get from top of stack */

        STACK(sp,variable);

}/* store_variable */

/*
 * conditional_jump
 *
 * Take a jump after an instruction based on the flag, either true or false. The
 * jump can be modified by the change logic flag. Normally jumps are taken
 * when the flag is true. When the change logic flag is set then the jump is
 * taken when flag is false. A PC relative jump can also be taken. This jump can
 * either be a positive or negative byte or word range jump. An additional
 * feature is the return option. If the jump offset is zero or one then that
 * literal value is passed to the return instruction, instead of a jump being
 * taken. Complicated or what!
 *
 */

#ifdef __STDC__
void conditional_jump (int flag)
#else
void conditional_jump (flag)
int flag;
#endif
{
    zbyte_t specifier;
    zword_t offset;

    /* Read the specifier byte */

    specifier = read_code_byte ();

    /* If the reverse logic flag is set then reverse the flag */

    if (specifier & 0x80)
        flag = (flag) ? 0 : 1;

    /* Jump offset is in bottom 6 bits */

    offset = (zword_t) specifier & 0x3f;

    /* If the byte range jump flag is not set then load another offset byte */

    if ((specifier & 0x40) == 0) {

        /* Add extra offset byte to existing shifted offset */

        offset = (offset << 8) + read_code_byte ();

        /* If top bit of offset is set then propogate the sign bit */

        if (offset & 0x2000)
            offset |= 0xc000;
    }

    /* If the flag is false then do the jump */

    if (flag == 0)
    {
        /* If offset equals 0 or 1 return that value instead */

        if (offset == 0 || offset == 1)
            ret (offset);
        else

            /* Add offset to PC */

            pc = (unsigned long) (pc + (short) offset - 2);
    }

}/* conditional_jump */
