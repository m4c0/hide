#pragma leco app

import casein;
import hai;
import vinyl;
import voo;

vinyl::v<struct as, struct ss> g;

struct as {
  voo::device_and_queue dq { "poc", casein::native_ptr };
};
struct ss {
  vee::render_pass rp = voo::single_att_render_pass(g.as()->dq);
  voo::swapchain sw { g.as()->dq };
  voo::single_cb cb {};
  hai::array<vee::framebuffer> fbs = sw.create_framebuffers(*rp);
  hai::array<vee::render_pass_begin> rpbs = [this] {
    hai::array<vee::render_pass_begin> res { fbs.size() };
    for (auto i = 0; i < res.size(); i++) 
      res[i] = vee::render_pass_begin {
        .command_buffer = cb.cb(),
        .render_pass = *rp,
        .framebuffer = *fbs[i],
        .extent = sw.extent(),
        .clear_colours { vee::clear_colour(0.01, 0.02, 0.03, 1.0) },
      };
    return res;
  }();
};

static void on_frame() {
  auto cb = g.ss()->cb.cb();

  voo::ots_present_guard pg { &g.ss()->sw, cb };
  voo::cmd_render_pass rp { g.ss()->rpbs[g.ss()->sw.index()] };
}

const int i = [] {
  g.on_frame() = on_frame;
  g.setup();
  return 0;
}();

