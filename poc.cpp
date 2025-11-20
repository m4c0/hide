#pragma leco app

import casein;
import hai;
import vinyl;
import voo;

struct as {
  voo::device_and_queue dq { "poc", casein::native_ptr };
};
static hai::uptr<as> gas {};

struct ss {
  vee::render_pass rp = voo::single_att_render_pass(gas->dq);
  voo::swapchain_and_stuff sw { gas->dq, *rp };
};
static hai::uptr<ss> gss {};

static void on_frame() {
}

static void setup() {
  using namespace vinyl;

  on(START,  [] { gas.reset(new as {}); });
  on(RESIZE, [] { gss.reset(nullptr); });
  on(FRAME,  [] {
    if (!gss) gss.reset(new ss {});
    on_frame();
  });
  on(STOP, [] {
    gss.reset(nullptr);
    gas.reset(nullptr);
  });
}

const int i = [] {
  setup();
  return 0;
}();

