#pragma once

/* Doubly linked list.

   This implementation of a doubly linked list does not require
   use of dynamically allocated memory.  Instead, each structure
   that is a potential list element must embed a node
   member.  All of the list functions operate on these `struct
   node's.  The entry macro allows conversion from a
   node back to a structure object that contains it.

   For example, suppose there is a needed for a list of `struct
   foo'.  `foo' should contain a `node'
   member, like so:

      foo
        {
          node elem;
          int bar;
          ...other members...
        };

   Then a list of `foo' can be be declared and initialized
   like so:

      list foo_list;

      init (&foo_list);

   Iteration is a typical situation where it is necessary to
   convert from a node back to its enclosing
   structure.  Here's an example using foo_list:

      node* e;

      for (e = begin (&foo_list); e != end (&foo_list);
           e = next (e))
        {
          foo *f = entry (e, foo, elem);
          ...do something with f...
        }

   You can find real examples of list usage throughout the
   source; for example, malloc.c, palloc.c, and thread.c in the
   threads directory all use lists.

   The interface for this list is inspired by the list<> template
   in the C++ STL.  If you're familiar with list<>, you should
   find this easy to use.  However, it should be emphasized that
   these lists do *no* type checking and can't do much other
   correctness checking.  If you screw up, it will bite you.

   Glossary of list terms:

     - "front": The first element in a list.  Undefined in an
       empty list.  Returned by front().

     - "back": The last element in a list.  Undefined in an empty
       list.  Returned by back().

     - "tail": The element figuratively just after the last
       element of a list.  Well defined even in an empty list.
       Returned by end().  Used as the end sentinel for an
       iteration from front to back.

     - "beginning": In a non-empty list, the front.  In an empty
       list, the tail.  Returned by begin().  Used as the
       starting point for an iteration from front to back.

     - "head": The element figuratively just before the first
       element of a list.  Well defined even in an empty list.
       Returned by rend().  Used as the end sentinel for an
       iteration from back to front.

     - "reverse beginning": In a non-empty list, the back.  In an
       empty list, the head.  Returned by rbegin().  Used as
       the starting point for an iteration from back to front.

     - "interior element": An element that is not the head or
       tail, that is, a real list element.  An empty list does
       not have any interior elements.
*/

#include <stddef.h>
#include <stdint.h>

/* Converts pointer to list element LIST_ELEM into a pointer to
the structure that LIST_ELEM is embedded inside.  Supply the
name of the outer structure and the member name MEMBER
of the list element.  See the big comment at the top of the
file for an example. */
#define entry(LIST_ELEM, STRUCT, MEMBER)           \
        ((*) ((uint8_t *) &(LIST_ELEM)->next            \
                     - offsetof (STRUCT, MEMBER.next)))

namespace list {

/* List element. */
struct node {
  node* prev;     /* Previous list element. */
  node* next;     /* Next list element. */
};

/* List. */
struct list {
  node head;      /* List head. */
  node tail;      /* List tail. */
};

/* List initialization.

    A list may be initialized by calling init():

        list my_list;
        init (&my_list); */

void init(list* list);

/* List traversal. */
node* begin(list* list);
node* next(node* );
node* end(list* list);

node* rbegin(list* list);
node* prev(node* );
node* rend(list* list);

node* head(list* list);
node* tail(list* list);

/* List insertion. */
void insert(node* , node* );
void splice(node* before,
  node* first, node* last);
void push_front(list* list, node* );
void push_back(list* list, node* );

/* List removal. */
node* remove(node* );
node* pop_front(list* list);
node* pop_back(list* list);

/* List elements. */
node* front(list* list);
node* back(list* list);

/* List properties. */
size_t size(list* list);
bool empty(list* list);

/* Miscellaneous. */
void reverse(list* list);

/* Compares the value of two list elements A and B, given
    auxiliary data AUX.  Returns true if A is less than B, or
    false if A is greater than or equal to B. */
typedef bool less_func(const node* a,
  const node* b,
  void *aux);

/* Operations on lists with ordered elements. */
void sort(list* list,
  less_func *, void *aux);
void insert_ordered(list* list, node* ,
  less_func *, void *aux);
void unique(list* orig, list* dupes,
  less_func *, void *aux);

/* Max and min. */
node* lmax(list* list, less_func *, void *aux);
node* lmin(list* list, less_func *, void *aux);

}

