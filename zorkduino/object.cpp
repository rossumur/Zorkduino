/*
 * object.c
 *
 * Object manipulation routines.
 *
 */

#include "ztypes.h"

#define PARENT 0
#define NEXT 1
#define CHILD 2

#ifdef __STDC__
static zword_t read_object (zword_t objp, int field);
static void write_object (zword_t objp, int field, zword_t value);
#else
static zword_t read_object ();
static void write_object ();
#endif

/*
 * get_object_address
 *
 * Calculate the address of an object in the data area.
 *
 */

#ifdef __STDC__
zword_t get_object_address (zword_t obj)
#else
zword_t get_object_address (obj)
zword_t obj;
#endif
{
    int offset;

    /* Address calculation is object table base + size of default properties area +
       object number-1 * object size */

    if (h_type < V4)
        offset = h_objects_offset + ((P3_MAX_PROPERTIES - 1) * 2) + ((obj - 1) * O3_SIZE);
    else
        offset = h_objects_offset + ((P4_MAX_PROPERTIES - 1) * 2) + ((obj - 1) * O4_SIZE);

    return ((zword_t) offset);

}/* get_object_address */

/*
 * insert_object
 *
 * Insert object 1 as the child of object 2 after first removing it from its
 * previous parent. The object is inserted at the front of the child object
 * chain.
 *
 */

#ifdef __STDC__
void insert_object (zword_t obj1, zword_t obj2)
#else
void insert_object (obj1, obj2)
zword_t obj1;
zword_t obj2;
#endif
{
    zword_t obj1p, obj2p, child2;

    /* Get addresses of both objects */

    obj1p = get_object_address (obj1);
    obj2p = get_object_address (obj2);

    /* Remove object 1 from current parent */

    remove_object (obj1);

    /* Make object 2 object 1's parent */

    write_object (obj1p, PARENT, obj2);

    /* Get current first child of object 2 */

    child2 = read_object (obj2p, CHILD);

    /* Make object 1 first child of object 2 */

    write_object (obj2p, CHILD, obj1);

    /* If object 2 had children then link them into the next child field of object 1 */

    if (child2)
        write_object (obj1p, NEXT, child2);

}/* insert_object */

/*
 * remove_object
 *
 * Remove an object by unlinking from the its parent object and from its
 * siblings.
 *
 */

#ifdef __STDC__
void remove_object (zword_t obj)
#else
void remove_object (obj)
zword_t obj;
#endif
{
    zword_t objp, parentp, childp, parent, child;

    /* Get address of object to be removed */

    objp = get_object_address (obj);

    /* Get parent of object, and return if no parent */

    if ((parent = read_object (objp, PARENT)) == 0)
        return;

    /* Get address of parent object */

    parentp = get_object_address (parent);

    /* Find first child of parent */

    child = read_object (parentp, CHILD);

    /* If object is first child then just make the parent child pointer
       equal to the next child */

    if (child == obj)
        write_object (parentp, CHILD, read_object (objp, NEXT));
    else {

        /* Walk down the child chain looking for this object */

        do {
            childp = get_object_address (child);
            child = read_object (childp, NEXT);
        } while (child != obj);

        /* Set the next pointer thre previous child to the next pointer
           of the current object child pointer */

        write_object (childp, NEXT, read_object (objp, NEXT));
    }

    /* Set the parent and next child pointers to NULL */

    write_object (objp, PARENT, 0);
    write_object (objp, NEXT, 0);

}/* remove_object */

/*
 * load_parent_object
 *
 * Load the parent object pointer of an object
 *
 */

#ifdef __STDC__
void load_parent_object (zword_t obj)
#else
void load_parent_object (obj)
zword_t obj;
#endif
{
                                       
    store_operand (read_object (get_object_address (obj), PARENT));

}/* load_parent_object */

/*
 * load_child_object
 *
 * Load the child object pointer of an object and jump if the child pointer is
 * not NULL.
 *
 */

#ifdef __STDC__
void load_child_object (zword_t obj)
#else
void load_child_object (obj)
zword_t obj;
#endif
{
    zword_t child;

    child = read_object (get_object_address (obj), CHILD);

    store_operand (child);

    conditional_jump (child != 0);

}/* load_child_object */

/*
 * load_next_object
 *
 * Load the next child object pointer of an object and jump if the next child
 * pointer is not NULL.
 *
 */

#ifdef __STDC__
void load_next_object (zword_t obj)
#else
void load_next_object (obj)
zword_t obj;
#endif
{
    zword_t next;

    next = read_object (get_object_address (obj), NEXT);

    store_operand (next);

    conditional_jump (next != 0);

}/* load_next_object */

/*
 * compare_parent_object
 *
 * Jump if object 2 is the parent of object 1
 *
 */

#ifdef __STDC__
void compare_parent_object (zword_t obj1, zword_t obj2)
#else
void compare_parent_object (obj1, obj2)
zword_t obj1;
zword_t obj2;
#endif
{

    conditional_jump (read_object (get_object_address (obj1), PARENT) == obj2);

}/* compare_parent_object */

/*
 * test_attr
 *
 * Test if an attribute bit is set.
 *
 */

#ifdef __STDC__
void test_attr (zword_t obj, zword_t bit)
#else
void test_attr (obj, bit)
zword_t obj;
zword_t bit;
#endif
{
    zword_t objp;
    zbyte_t value;

    assert (O3_ATTRIBUTES == O4_ATTRIBUTES);

    /* Get attribute address */

    objp = get_object_address (obj) + (bit >> 3);

    /* Load attribute byte */

    value = get_byte (objp);

    /* Test attribute */

    conditional_jump ((value >> (7 - (bit & 7))) & 1);

}/* test_attr */

/*
 * set_attr
 *
 * Set an attribute bit.
 *
 */

#ifdef __STDC__
void set_attr (zword_t obj, zword_t bit)
#else
void set_attr (obj, bit)
zword_t obj;
zword_t bit;
#endif
{
    zword_t objp;
    zbyte_t value;

    assert (O3_ATTRIBUTES == O4_ATTRIBUTES);

    /* Get attribute address */

    objp = get_object_address (obj) + (bit >> 3);

    /* Load attribute byte */

    value = get_byte (objp);

    /* Set attribute bit */

    value |= (zbyte_t) (1 << (7 - (bit & 7)));

    /* Store attribute byte */

    set_byte (objp, value);

}/* set_attr */

/*
 * clear_attr
 *
 * Clear an attribute bit
 *
 */

#ifdef __STDC__
void clear_attr (zword_t obj, zword_t bit)
#else
void clear_attr (obj, bit)
zword_t obj;
zword_t bit;
#endif
{
    zword_t objp;
    zbyte_t value;

    assert (O3_ATTRIBUTES == O4_ATTRIBUTES);

    /* Get attribute address */

    objp = get_object_address (obj) + (bit >> 3);

    /* Load attribute byte */

    value = get_byte (objp);

    /* Clear attribute bit */

    value &= (zbyte_t) ~(1 << (7 - (bit & 7)));

    /* Store attribute byte */

    set_byte (objp, value);

}/* clear_attr */

#ifdef __STDC__
static zword_t read_object (zword_t objp, int field)
#else
static zword_t read_object (objp, field)
zword_t objp;
int field;
#endif
{
    zword_t value;

    if (h_type < V4) {
        if (field == PARENT)
            value = (zword_t) get_byte (PARENT3 (objp));
        else if (field == NEXT)
            value = (zword_t) get_byte (NEXT3 (objp));
        else
            value = (zword_t) get_byte (CHILD3 (objp));
    } else {
        if (field == PARENT)
            value = get_word (PARENT4 (objp));
        else if (field == NEXT)
            value = get_word (NEXT4 (objp));
        else
            value = get_word (CHILD4 (objp));
    }

    return (value);

}/* read_object */

#ifdef __STDC__
static void write_object (zword_t objp, int field, zword_t value)
#else
static void write_object (objp, field, value)
zword_t objp;
int field;
zword_t value;
#endif
{

    if (h_type < V4) {
        if (field == PARENT)
            set_byte (PARENT3 (objp), value);
        else if (field == NEXT)
            set_byte (NEXT3 (objp), value);
        else
            set_byte (CHILD3 (objp), value);
    } else {
        if (field == PARENT)
            set_word (PARENT4 (objp), value);
        else if (field == NEXT)
            set_word (NEXT4 (objp), value);
        else
            set_word (CHILD4 (objp), value);
    }

}/* write_object */
