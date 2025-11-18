#pragma leco tool
#pragma leco add_impl microui

import hay;
import print;

extern "C" {
#include "microui/src/microui.h"
}

static auto init() {
  auto ctx = new mu_Context {};
  mu_init(ctx);
  return ctx;
}
static auto deinit(mu_Context * ptr) {
  delete ptr;
}

static int text_width(mu_Font, const char *, int len) {
  return 5 * len;
}
static int text_height(mu_Font) {
  return 8;
}

int main() {
  hay<mu_Context *, init, deinit> ctx {}; 
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  mu_begin(ctx);

  auto wnd_rect = mu_rect(10, 10, 300, 400);
  //auto wnd_opts = MU_OPT_NOCLOSE | MU_OPT_NOTITLE;
  auto wnd_opts = 0;
  if (mu_begin_window_ex(ctx, "Window", wnd_rect, wnd_opts)) {
    int cols[] { 100, 100 };
    mu_layout_row(ctx, 2, cols, 0);

    mu_text(ctx, "One");
    if (mu_button(ctx, "Hello")) putln("hello");

    mu_text(ctx, "Two");
    if (mu_button(ctx, "World")) putln("world");

    mu_end_window(ctx);
  }

  mu_end(ctx);

  mu_Command * cmd {};
  while (mu_next_command(ctx, &cmd)) {
    switch (cmd->type) {
      case MU_COMMAND_CLIP: {
        auto [x, y, w, h] = cmd->clip.rect;
        putan("clip", x, y, w, h);
        break;
      }
      case MU_COMMAND_ICON: {
        auto [x, y, w, h] = cmd->icon.rect;
        putan("icon", cmd->icon.id, x, y, w, h);
        break;
      }
      case MU_COMMAND_RECT: {
        auto [x, y, w, h] = cmd->rect.rect;
        putan("rect", x, y, w, h);
        break;
      }
      case MU_COMMAND_TEXT:
        putan("text", cmd->text.pos.x, cmd->text.pos.y, cmd->text.str);
        break;
      default:
        putan("cmd", cmd->type);
        break;
    }
  }
}
