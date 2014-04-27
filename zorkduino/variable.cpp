/*
 * variable.c
 *
 * Variable manipulation routines
 *
 */

#include "ztypes.h"

/*
 * load
 *
 * Load and store a variable value.
 *
 */

#ifdef __STDC__
void load (zword_t variable)
#else
void load (variable)
zword_t variable;
#endif
{

    store_operand (load_variable (variable));

}/* load */

/*
 * push_var
 *
 * Push a value onto the stack
 *
 */

#ifdef __STDC__
void push_var (zword_t value)
#else
void push_var (value)
zword_t value;
#endif
{

    PUSH(value);

}/* push_var */

/*
 * pop_var
 *
 * Pop a variable from the stack.
 *
 */

#ifdef __STDC__
void pop_var (zword_t variable)
#else
void pop_var (variable)
zword_t variable;
#endif
{

    store_variable (variable, POP());

}/* pop_var */

/*
 * increment
 *
 * Increment a variable.
 *
 */

#ifdef __STDC__
void increment (zword_t variable)
#else
void increment (variable)
zword_t variable;
#endif
{

    store_variable (variable, load_variable (variable) + 1);

}/* increment */

/*
 * decrement
 *
 * Decrement a variable.
 *
 */

#ifdef __STDC__
void decrement (zword_t variable)
#else
void decrement (variable)
zword_t variable;
#endif
{

    store_variable (variable, load_variable (variable) - 1);

}/* decrement */

/*
 * increment_check
 *
 * Increment a variable and then check its value against a target.
 *
 */

#ifdef __STDC__
void increment_check (zword_t variable, zword_t target)
#else
void increment_check (variable, target)
zword_t variable;
zword_t target;
#endif
{
    short value;

    value = (short) load_variable (variable);
    store_variable (variable, ++value);
    conditional_jump (value > (short) target);

}/* increment_check */

/*
 * decrement_check
 *
 * Decrement a variable and then check its value against a target.
 *
 */

#ifdef __STDC__
void decrement_check (zword_t variable, zword_t target)
#else
void decrement_check (variable, target)
zword_t variable;
zword_t target;
#endif
{
    short value;

    value = (short) load_variable (variable);
    store_variable (variable, --value);
    conditional_jump (value < (short) target);

}/* decrement_check */
