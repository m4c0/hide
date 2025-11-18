#pragma leco tool
#pragma leco add_impl microui

import hay;
import print;

extern "C" {
#include "microui/src/microui.h"
}

static auto init_mu_ctx() {
  auto ctx = new mu_Context {};
  mu_init(ctx);
  return ctx;
}
static auto deinit_mu_ctx(mu_Context * ptr) {
  delete ptr;
}

int main() {
  hay<mu_Context *, init_mu_ctx, deinit_mu_ctx> ctx {}; 
}
