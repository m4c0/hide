#pragma leco add_impl microui
module;

extern "C" {
#include "microui/src/microui.h"
}

export module hide:microui;
import dotz;
import hai;
import hay;
import sv;

export namespace hide::commands {
  using clip = mu_ClipCommand;
  using icon = mu_IconCommand;
  using rect = mu_RectCommand;
  using text = mu_TextCommand;
}

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

  export void for_each_command(auto && fn) {
    mu_Command * cmd {};
    while (mu_next_command(context(), &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_CLIP: fn(cmd->clip); break;
        case MU_COMMAND_ICON: fn(cmd->icon); break;
        case MU_COMMAND_RECT: fn(cmd->rect); break;
        case MU_COMMAND_TEXT: fn(cmd->text); break;
      }
    }
  }
  export void for_each_command(auto && clip, auto && icon, auto && rect, auto && text) {
    using C = traits::decay_t<decltype(clip)>;
    using I = traits::decay_t<decltype(icon)>;
    using R = traits::decay_t<decltype(rect)>;
    using T = traits::decay_t<decltype(text)>;

    struct t : C, I, R, T {
      using C::operator();
      using I::operator();
      using R::operator();
      using T::operator();
    } t { clip, icon, rect, text };
    for_each_command(t);
  }
}

export namespace hide::mu {
  inline bool button(const char * str) {
    return mu_button(context(), str);
  }
  inline void layout_row(hai::view<int> widths, int height) {
    mu_layout_row(context(), widths.size(), widths.begin(), height);
  }
  inline void text(const char * str) {
    mu_text(context(), str);
  }
}
