#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"

import casein;
import dotz;
import hai;
import vinyl;
import voo;

static constexpr const auto max_inst = 10240;
struct inst {
  dotz::vec2 pos;
};

struct upc {
  dotz::vec2 extent;
};

vinyl::v<struct as, struct ss> g;
struct as {
  voo::device_and_queue dq { "poc", casein::native_ptr };
  voo::one_quad quad { dq };
  voo::bound_buffer buf = voo::bound_buffer::create_from_host(
      dq.physical_device(), sizeof(inst) * max_inst,
      vee::buffer_usage::vertex_buffer);
};
struct ss {
  voo::swapchain sw { g.as()->dq };

  voo::single_cb cb {};
  vee::render_pass rp = voo::single_att_render_pass(g.as()->dq);
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

  vee::pipeline_layout pl = vee::create_pipeline_layout(
      vee::vertex_push_constant_range<upc>());
  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *pl,
    .render_pass = *rp,
    .extent = sw.extent(),
    .shaders {
      voo::shader("poc.vert.spv").pipeline_vert_stage("main"),
      voo::shader("poc.frag.spv").pipeline_frag_stage("main"),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(float) * 2),
      vee::vertex_input_bind_per_instance(sizeof(inst)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
      vee::vertex_attribute_vec2(1, traits::offset_of(&inst::pos)),
    },
  });
};

static void on_frame() {
  auto cb = g.ss()->cb.cb();
  auto aspect = g.ss()->sw.aspect();

  upc pc {
    .extent { aspect * 10.f, 10.f },
  };

  unsigned count = 0;
  {
    voo::memiter<inst> m { *g.as()->buf.memory, &count };
    m += {
      .pos { 0, 0 },
    };
    m += {
      .pos { 9, 9 },
    };
  }

  voo::ots_present_guard pg { &g.ss()->sw, cb };
  voo::cmd_render_pass rp { g.ss()->rpbs[g.ss()->sw.index()] };
  vee::cmd_bind_gr_pipeline(cb, *g.ss()->gp);
  vee::cmd_bind_vertex_buffers(cb, 1, *g.as()->buf.buffer);
  vee::cmd_push_vertex_constants(cb, *g.ss()->pl, &pc);
  g.as()->quad.run(cb, 0, count);
}

const int i = [] {
  g.on_frame() = on_frame;
  g.setup();
  return 0;
}();

