#pragma once
#include "fixed.h"

struct Size {
  Size() = default;
  Size( unsigned w, unsigned h, unsigned d = 0) : 
    w( w << FIXEDPT_DEC_BITS ),
    h( h << FIXEDPT_DEC_BITS ),
    d( d << FIXEDPT_DEC_BITS ) { }
  unsigned w{};
  unsigned h{};
  unsigned d{};
};
