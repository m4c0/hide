#pragma leco add_impl microui
module;

extern "C" {
#include "microui/src/microui.h"
}

export module hide;
import dotz;
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

  export auto window(dotz::vec2 pos, dotz::vec2 size) {
    auto rect = mu_rect(pos.x, pos.y, size.x, size.y);
    auto opts = MU_OPT_NOCLOSE | MU_OPT_NOTITLE;
    return hay<int, mu_begin_window_ex, [](int b) {
      if (b) mu_end_window(context());
    }> { context(), "", rect, opts };
  }
}
