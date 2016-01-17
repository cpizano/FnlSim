#include "stdafx.h"

#include "handle.h"
#include "kheap.h"
#include "object.h"

namespace handle {

const uint32_t first_lev_count = 64U;

Entry* make_table() {
  Entry* e = (Entry*)kheap::alloc(sizeof(Entry) * first_lev_count);
  e[0].type = obj::sentry;
  return e;
}

void add_obj(obj::Base* ob, handle::Entry* table) {
  uint32_t ix = table[0].ob_rel;

  for (; ix != first_lev_count; ++ix) {
    if (table[ix].type == obj::vacant) {
      table[ix].type = ob->type;
      table[ix].ob_rel = kheap::get_reladdr(ob);
      table[0].ob_rel = (ix + 1) % first_lev_count;
      return;
    }
  }
  KPANIC;
}

}