#pragma leco app
#pragma leco add_shader "poc-2.vert"
#pragma leco add_shader "poc-2.frag"

import casein;
import dotz;
import hai;
import vinyl;
import voo;

struct inst {
  dotz::vec2 pos;
  dotz::vec4 colour;
};
static constexpr const auto max_inst = 1024;

struct as {
  voo::device_and_queue dq { "poc2-hide", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq);

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  voo::bound_buffer buf = voo::bound_buffer::create_from_host(
      dq.physical_device(), max_inst * sizeof(inst),
      vee::buffer_usage::vertex_buffer);

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
      voo::shader { "poc-2.vert.spv" }.pipeline_vert_stage("main", vee::specialisation_info<float>(0, sw.aspect())),
      voo::shader { "poc-2.frag.spv" }.pipeline_frag_stage("main"),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
      vee::vertex_input_bind_per_instance(sizeof(inst)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
      vee::vertex_attribute_vec2(1, traits::offset_of(&inst::pos)),
      vee::vertex_attribute_vec4(1, traits::offset_of(&inst::colour)),
    },
  });
};
hai::uptr<ss> gss {};

static void on_frame() {
  if (!gss) gss.reset(new ss {});

  unsigned count = 0;
  {
    voo::memiter<inst> m { *gas->buf.memory, &count };
    m += inst {
      .pos { -1, -1 },
      .colour { 0, 1, 1, 1 },
    };
    m += inst {
      .pos { -0, -0 },
      .colour { 1, 1, 1, 1 },
    };
  }

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [&] {
    auto cb = gss->sw.command_buffer();
    auto rp = gss->sw.cmd_render_pass({
      .clear_colours { vee::clear_colour(0.01f, 0.02f, 0.03f, 1.0f) },
    });
    vee::cmd_bind_gr_pipeline(cb, *gss->gp);
    vee::cmd_bind_vertex_buffers(cb, 1, *gas->buf.buffer);
    gas->quad.run(cb, 0, count);
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
