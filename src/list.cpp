#include "stdafx.h"

#include "list.h"

/* Our doubly linked lists have two header elements: the "head"
   just before the first element and the "tail" just after the
   last element.  The `prev' link of the front header is null, as
   is the `next' link of the back header.  Their other two links
   point toward each other via the interior elements of the list.

   An empty list looks like this:

                      +------+     +------+
                  <---| head |<--->| tail |--->
                      +------+     +------+

   A list with two elements in it looks like this:

        +------+     +-------+     +-------+     +------+
    <---| head |<--->|   1   |<--->|   2   |<--->| tail |<--->
        +------+     +-------+     +-------+     +------+

   The symmetry of this arrangement eliminates lots of special
   cases in list processing.  For example, take a look at
   remove(): it takes only two pointer assignments and no
   conditionals.  That's a lot simpler than the code would be
   without header elements.

   (Because only one of the pointers in each header element is used,
   we could in fact combine them into a single header element
   without sacrificing this simplicity.  But using two separate
   elements allows us to do a little bit of checking on some
   operations, which can be valuable.) */

namespace {

/* Returns true only if the list elements A through B (exclusive)
are in order according to LESS given auxiliary data AUX. */
static bool is_sorted(list::node* a, list::node* b,
                      list::less_func *less, void *aux) {
  if (a != b) {
    while ((a = next(a)) != b)
      if (less(a, prev(a), aux))
        return false;
  }
  return true;
}

/* Finds a run, starting at A and ending not after B, of list
elements that are in nondecreasing order according to LESS
given auxiliary data AUX.  Returns the (exclusive) end of the
run.
A through B (exclusive) must form a non-empty range. */
static list::node* find_end_of_run(list::node* a, list::node* b,
                                   list::less_func *less, void *aux) {
  // ASSERT (a != b);
  do {
    a = next(a);
  } while (a != b && !less(a, prev(a), aux));
  return a;
}

/* Merges A0 through A1B0 (exclusive) with A1B0 through B1
(exclusive) to form a combined range also ending at B1
(exclusive).  Both input ranges must be nonempty and sorted in
nondecreasing order according to LESS given auxiliary data
AUX.  The output range will be sorted the same way. */
static void inplace_merge(list::node* a0, list::node* a1b0, list::node* b1,
                          list::less_func *less, void *aux) {
  // ASSERT (is_sorted (a0, a1b0, less, aux));
  // ASSERT (is_sorted (a1b0, b1, less, aux));

  while (a0 != a1b0 && a1b0 != b1)
    if (!less(a1b0, a0, aux)) {
      a0 = next(a0);
    } else {
      a1b0 = next(a1b0);
      splice(a0, prev(a1b0), a1b0);
    }
}

/* Returns true if ELEM is a head, false otherwise. */
inline bool is_head(list::node* elem) {
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

/* Returns true if ELEM is an interior element,
false otherwise. */
inline bool is_interior(list::node* elem) {
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

/* Returns true if ELEM is a tail, false otherwise. */
inline bool is_tail(list::node* elem) {
  return elem != NULL && elem->prev != NULL && elem->next == NULL;
}

}   // namespace.


namespace list {

/* Initializes LIST as an empty list. */
void init(list *list) {
  list->head.prev = NULL;
  list->head.next = &list->tail;
  list->tail.prev = &list->head;
  list->tail.next = NULL;
}

/* Returns the beginning of LIST.  */
node* begin(list *list) {
  return list->head.next;
}

/* Returns the element after ELEM in its list.  If ELEM is the
    last element in its list, returns the list tail.  Results are
    undefined if ELEM is itself a list tail. */
node* next(node* elem) {
  // ASSERT (is_head (elem) || is_interior (elem));
  return elem->next;
}

/* Returns LIST's tail.

    end() is often used in iterating through a list from
    front to back.  See the big comment at the top of list.h for
    an example. */
node* end(list *list) {
  return &list->tail;
}

/* Returns the LIST's reverse beginning, for iterating through
    LIST in reverse order, from back to front. */
node* rbegin(list *list) {
  return list->tail.prev;
}

/* Returns the element before ELEM in its list.  If ELEM is the
    first element in its list, returns the list head.  Results are
    undefined if ELEM is itself a list head. */
node* prev(node* elem) {
  // ASSERT (is_interior (elem) || is_tail (elem));
  return elem->prev;
}

/* Returns LIST's head.

    rend() is often used in iterating through a list in
    reverse order, from back to front.  Here's typical usage,
    following the example from the top of list.h:

      for (e = rbegin (&foo_list); e != rend (&foo_list);
            e = prev (e))
        {
          foo *f = entry (e, foo, elem);
          ...do something with f...
        }
*/
node* rend(list *list) {
  return &list->head;
}

/* Return's LIST's head.

    head() can be used for an alternate style of iterating
    through a list, e.g.:

      e = head (&list);
      while ((e = next (e)) != end (&list))
        {
          ...
        }
*/
node* head(list *list) {
  return &list->head;
}

/* Return's LIST's tail. */
node* tail(list *list) {
  return &list->tail;
}

/* Inserts ELEM just before BEFORE, which may be either an
    interior element or a tail.  The latter case is equivalent to
    push_back(). */
void insert(node* before, node* elem) {
  //ASSERT (is_interior (before) || is_tail (before));
  //ASSERT (elem != NULL);

  elem->prev = before->prev;
  elem->next = before;
  before->prev->next = elem;
  before->prev = elem;
}

/* Removes elements FIRST though LAST (exclusive) from their
    current list, then inserts them just before BEFORE, which may
    be either an interior element or a tail. */
void splice(node* before, node* first, node* last) {
  //ASSERT (is_interior (before) || is_tail (before));
  if (first == last)
    return;
  last = prev(last);

  //ASSERT (is_interior (first));
  //ASSERT (is_interior (last));

  /* Cleanly remove FIRST...LAST from its current list. */
  first->prev->next = last->next;
  last->next->prev = first->prev;

  /* Splice FIRST...LAST into new list. */
  first->prev = before->prev;
  last->next = before;
  before->prev->next = first;
  before->prev = last;
}

/* Inserts ELEM at the beginning of LIST, so that it becomes the
    front in LIST. */
void push_front(list *list, node* elem) {
  insert(begin(list), elem);
}

/* Inserts ELEM at the end of LIST, so that it becomes the
    back in LIST. */
void push_back(list *list, node* elem) {
  insert(end(list), elem);
}

/* Removes ELEM from its list and returns the element that
    followed it.  Undefined behavior if ELEM is not in a list.

    A list element must be treated very carefully after removing
    it from its list.  Calling next() or prev() on ELEM
    will return the item that was previously before or after ELEM,
    but, e.g., prev(next(ELEM)) is no longer ELEM!

    The remove() return value provides a convenient way to
    iterate and remove elements from a list:

    for (e = begin (&list); e != end (&list); e = remove (e))
      {
        ...do something with e...
      }

    If you need to free() elements of the list then you need to be
    more conservative.  Here's an alternate strategy that works
    even in that case:

    while (!empty (&list))
      {
        node* e = pop_front (&list);
        ...do something with e...
      }
*/
node* remove(node* elem) {
  //ASSERT (is_interior (elem));
  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return elem->next;
}

/* Removes the front element from LIST and returns it.
    Undefined behavior if LIST is empty before removal. */
node* pop_front(list *list) {
  node* f = front(list);
  remove(f);
  return f;
}

/* Removes the back element from LIST and returns it.
    Undefined behavior if LIST is empty before removal. */
node* pop_back(list *list) {
  node* b = back(list);
  remove(b);
  return b;
}

/* Returns the front element in LIST.
    Undefined behavior if LIST is empty. */
node* front(list *list) {
  // ASSERT (!empty (list));
  return list->head.next;
}

/* Returns the back element in LIST.
    Undefined behavior if LIST is empty. */
node* back(list *list) {
  // ASSERT (!empty (list));
  return list->tail.prev;
}

/* Returns the number of elements in LIST.
    Runs in O(n) in the number of elements. */
size_t size(list *list) {
  node* e;
  size_t cnt = 0;

  for (e = begin(list); e != end(list); e = next(e)) {
    cnt++;
  }
  return cnt;
}

/* Returns true if LIST is empty, false otherwise. */
bool empty(list *list) {
  return begin(list) == end(list);
}

/* Swaps the `node* 's that A and B point to. */
static void swap(node** a, node** b) {
  node* t = *a;
  *a = *b;
  *b = t;
}

/* Reverses the order of LIST. */
void reverse(list *list) {
  if (!empty(list)) {
    node* e;

    for (e = begin(list); e != end(list); e = e->prev) {
      swap(&e->prev, &e->next);
    }
    swap(&list->head.next, &list->tail.prev);
    swap(&list->head.next->prev, &list->tail.prev->next);
  }
}

/* Sorts LIST according to LESS given auxiliary data AUX, using a
    natural iterative merge sort that runs in O(n lg n) time and
    O(1) space in the number of elements in LIST. */
void sort(list *list, less_func *less, void *aux) {
  size_t output_run_cnt;        /* Number of runs output in current pass. */

  /* Pass over the list repeatedly, merging adjacent runs of
      nondecreasing elements, until only one run is left. */
  do {
    node* a0;     /* Start of first run. */
    node* a1b0;   /* End of first run, start of second. */
    node* b1;     /* End of second run. */

    output_run_cnt = 0;
    for (a0 = begin(list); a0 != end(list); a0 = b1) {
      /* Each iteration produces one output run. */
      output_run_cnt++;

      /* Locate two adjacent runs of nondecreasing elements
          A0...A1B0 and A1B0...B1. */
      a1b0 = find_end_of_run(a0, end(list), less, aux);
      if (a1b0 == end(list))
        break;
      b1 = find_end_of_run(a1b0, end(list), less, aux);

      /* Merge the runs. */
      inplace_merge(a0, a1b0, b1, less, aux);
    }
  } while (output_run_cnt > 1);

  // ASSERT (is_sorted (begin (list), end (list), less, aux));
}

/* Inserts ELEM in the proper position in LIST, which must be
    sorted according to LESS given auxiliary data AUX.
    Runs in O(n) average case in the number of elements in LIST. */
void insert_ordered(list *list, node* elem,
    less_func *less, void *aux) {
  node* e;

  for (e = begin(list); e != end(list); e = next(e))
    if (less(elem, e, aux))
      break;
  return insert(e, elem);
}

/* Iterates through LIST and removes all but the first in each
    set of adjacent elements that are equal according to LESS
    given auxiliary data AUX.  If DUPLICATES is non-null, then the
    elements from LIST are appended to DUPLICATES. */
void unique(list* orig, list* dupes,
            less_func *less, void *aux) {
  node* elem, *nx;

  if (empty(orig))
    return;

  elem = begin(orig);
  while ((nx = next(elem)) != end(orig))
    if (!less(elem, nx, aux) && !less(nx, elem, aux)) {
      remove(nx);
      if (dupes != NULL)
        push_back(dupes, nx);
    } else {
      elem = nx;
    }
}

/* Returns the element in LIST with the largest value according
    to LESS given auxiliary data AUX.  If there is more than one
    maximum, returns the one that appears earlier in the list.  If
    the list is empty, returns its tail. */
node* lmax(list *list, less_func *less, void *aux) {
  node* max = begin(list);
  if (max != end(list)) {
    node* e;

    for (e = next(max); e != end(list); e = next(e)) {
      if (less(max, e, aux))
        max = e;
    }
  }
  return max;
}

/* Returns the element in LIST with the smallest value according
    to LESS given auxiliary data AUX.  If there is more than one
    minimum, returns the one that appears earlier in the list.  If
    the list is empty, returns its tail. */
node* lmin(list *list, less_func *less, void *aux) {
  node* min = begin(list);
  if (min != end(list)) {
    node* e;

    for (e = next(min); e != end(list); e = next(e)) {
      if (less(e, min, aux))
        min = e;
    }
  }
  return min;
}

} // namespace