#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"

import casein;
import dotz;
import hai;
import no;
import vinyl;
import voo;

static constexpr const auto max_inst = 10240;
struct inst {
  dotz::vec2 pos;
  dotz::vec2 size;
  dotz::vec4 colour;
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

  vee::render_pass rp = voo::single_att_render_pass(dq);

  vee::pipeline_layout pl = vee::create_pipeline_layout(
      vee::vertex_push_constant_range<upc>());
  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *pl,
    .render_pass = *rp,
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
      vee::vertex_attribute_vec2(1, traits::offset_of(&inst::size)),
      vee::vertex_attribute_vec4(1, traits::offset_of(&inst::colour)),
    },
  });
};
struct ss {
  voo::swapchain sw { g.as()->dq };

  voo::single_cb cb {};
  hai::array<vee::framebuffer> fbs = sw.create_framebuffers(*g.as()->rp);
  hai::array<vee::render_pass_begin> rpbs = [this] {
    hai::array<vee::render_pass_begin> res { fbs.size() };
    for (auto i = 0; i < res.size(); i++) 
      res[i] = vee::render_pass_begin {
        .command_buffer = cb.cb(),
        .render_pass = *g.as()->rp,
        .framebuffer = *fbs[i],
        .extent = sw.extent(),
        .clear_colours { vee::clear_colour(0.01, 0.02, 0.03, 1.0) },
      };
    return res;
  }();
};

class recorder : no::no {
  vee::command_buffer m_cb;
  float m_h;

  unsigned m_count = 0;
  unsigned m_first = 0;
  voo::memiter<inst> m { *g.as()->buf.memory, &m_count };
public:
  explicit recorder(vee::command_buffer cb, float h) : m_cb { cb }, m_h { h } {}

  void push(inst i) { m += i; }
  void run() {
    if (m_first == m_count) return;
    g.as()->quad.run(m_cb, 0, m_count - m_first, m_first);
    m_first = m_count;
  }
  void scissor(dotz::vec2 pos, dotz::vec2 size) {
    run();

    const auto bh = g.ss()->sw.extent().height / m_h;
    pos = pos * bh;
    size = size * bh;

    vee::cmd_set_scissor(m_cb, {
      { static_cast<int>(pos.x), static_cast<int>(pos.y) },
      { static_cast<unsigned>(size.x), static_cast<unsigned>(size.y) },
    });
  }
};

static void on_frame() {
  auto cb = g.ss()->cb.cb();
  auto aspect = g.ss()->sw.aspect();
  auto ext = g.ss()->sw.extent();

  auto h = 10.f;
  auto w = aspect * h;
  upc pc {
    .extent { w, h },
  };

  voo::ots_present_guard pg { &g.ss()->sw, cb };
  voo::cmd_render_pass rp { g.ss()->rpbs[g.ss()->sw.index()] };
  vee::cmd_set_viewport(cb, ext);
  vee::cmd_bind_gr_pipeline(cb, *g.as()->gp);
  vee::cmd_bind_vertex_buffers(cb, 1, *g.as()->buf.buffer);
  vee::cmd_push_vertex_constants(cb, *g.as()->pl, &pc);

  {
    recorder r { cb, h };
    r.scissor({ 0, 0 }, { w, h });

    r.push({
      .pos { 0, 0 },
      .size { 1 },
      .colour { 1, 0, 0, 1 },
    });
    r.push({
      .pos { 1, 0 },
      .size { w - 2.f, 1.f },
      .colour { 0.5f },
    });
    r.push({
      .pos { w - 1.f, 0.f },
      .size { 1 },
      .colour { 0, 1, 0, 1 },
    });

    r.push({
      .pos { 0, 1 },
      .size { w, h - 2.f },
      .colour { 0.2f },
    });
    r.scissor({ 0, 1 }, { w, h - 2 });

    for (auto y = 0.5f; y < h + 1; y += 0.9f) {
      r.push({
        .pos { 0.5f, y },
        .size { w - 1.f, 0.7f },
        .colour { 0.2f, 0.5f, 0.8f, 1.0f },
      });
    }

    r.run();
    r.scissor({ 0, 0 }, { w, h });

    r.push({
      .pos { 0.f, h - 1.f },
      .size { w / 5.0f, 1.f },
      .colour { 0.5f, 0.f, 0.f, 1.f },
    });
    r.push({
      .pos { w / 5.0f, h - 1.f },
      .size { w / 5.0f, 1.f },
      .colour { 0.f, 0.5f, 0.f, 1.f },
    });
    r.push({
      .pos { 2.0f * w / 5.0f, h - 1.f },
      .size { w / 5.0f, 1.f },
      .colour { 0.f, 0.f, 0.5f, 1.f },
    });
    r.push({
      .pos { 3.0f * w / 5.0f, h - 1.f },
      .size { w / 5.0f, 1.f },
      .colour { 0.5f, 0.5f, 0.f, 1.f },
    });
    r.push({
      .pos { 4.0f * w / 5.0f, h - 1.f },
      .size { w / 5.0f, 1.f },
      .colour { 0.5f, 0.f, 0.5f, 1.f },
    });

    r.run();
  }
}

const int i = [] {
  g.on_frame() = on_frame;
  g.setup();
  return 0;
}();

