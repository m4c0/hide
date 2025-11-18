#pragma leco add_impl microui
module;

extern "C" {
#include "microui/src/microui.h"
}

export module hide;
import hay;

namespace hide {
  export mu_Context * context() {
    static auto ctx = [] {
      hay<mu_Context *> ctx { new mu_Context {} }; 
      mu_init(ctx);
      return ctx;
    }();
    return ctx;
  }
}
