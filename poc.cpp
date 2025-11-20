#pragma leco app

import casein;
import vinyl;
import voo;

vinyl::v<struct as, struct ss> g;

struct as {
  voo::device_and_queue dq { "poc", casein::native_ptr };
};
struct ss {
  //vee::render_pass rp = voo::single_att_render_pass(g.as()->dq);
  voo::swapchain sw { g.as()->dq };
  voo::single_cb cb {};
};

static void on_frame() {
  auto cb = g.ss()->cb.cb();

  g.ss()->sw.acquire_next_image();
  {
    voo::cmd_buf_one_time_submit ots { cb };
  }
  g.ss()->sw.queue_submit(cb);
  g.ss()->sw.queue_present();
}

const int i = [] {
  g.on_frame() = on_frame;
  g.setup();
  return 0;
}();

