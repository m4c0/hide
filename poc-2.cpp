#pragma leco app
#pragma leco add_shader "poc-2.vert"
#pragma leco add_shader "poc-2.frag"

import casein;
import dotz;
import hai;
import vinyl;
import voo;

struct as {
  voo::device_and_queue dq { "poc2-hide", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq);

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  voo::one_quad quad { dq.physical_device() };
};
hai::uptr<as> gas {};

struct ss {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };

  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *gas->pl,
    .render_pass = *gas->rp,
    .extent = sw.extent(),
    .shaders {
      voo::shader { "poc-2.vert.spv" }.pipeline_vert_stage("main"),
      voo::shader { "poc-2.frag.spv" }.pipeline_frag_stage("main"),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
    },
  });
};
hai::uptr<ss> gss {};

static void on_frame() {
  if (!gss) gss.reset(new ss {});

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [] {
    auto cb = gss->sw.command_buffer();
    auto rp = gss->sw.cmd_render_pass();
    vee::cmd_bind_gr_pipeline(cb, *gss->gp);
    gas->quad.run(cb, 0);
  });
  gss->sw.queue_present(gas->dq.queue());
}

const int i = [] {
  using namespace vinyl;
  on(START, [] { gas.reset(new as {}); });
  on(STOP, [] { gss = {}; gas = {}; });
  on(FRAME, on_frame);
  on(RESIZE, [] { gss = {}; });
  return 0;
}();
