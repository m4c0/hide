#pragma leco add_impl microui
module;

extern "C" {
#include "microui/src/microui.h"
}

export module hide;
import hai;
import hay;
import sv;

namespace hide {
  export hai::fn<int, sv> text_width = [](auto) { return 0; };
  export hai::fn<int> text_height = [] { return 0; };

  export mu_Context * context() {
    static auto ctx = [] {
      hay<mu_Context *> ctx { new mu_Context {} }; 
      mu_init(ctx);
      ctx->text_width = [](mu_Font, const char * str, int len) {
        return text_width(sv { str, static_cast<unsigned>(len) });
      };
      ctx->text_height = [](mu_Font) {
        return text_height();
      };
      return ctx;
    }();
    return ctx;
  }

  export auto block() {
    return hay<mu_Context *,
      [] {
        mu_begin(context());
        return context();
      },
      mu_end> {};
  }
}
