#pragma once

namespace obj {
  struct Base;
}

namespace handle {

struct Entry {
  uint16_t type;
  uint16_t access;
  uint32_t ob_rel;
};

Entry* make_table();

void add_obj(obj::Base* ob, handle::Entry* table);

}
