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

class recorder : no::no {
  vee::command_buffer m_cb;
  float m_bh;

  unsigned m_count = 0;
  unsigned m_first = 0;
  voo::memiter<inst> m;
public:
  explicit recorder(vee::command_buffer cb, float bh, vee::device_memory::type mem) :
    m_cb { cb }
  , m_bh { bh }
  , m { mem, &m_count }
  {}
  ~recorder() {
    vee::end_cmd_buf(m_cb);
  }

  void push(inst i) { m += i; }
  void run() {
    if (m_first == m_count) return;
    vee::cmd_draw(m_cb, 6, m_count - m_first, m_first);
    m_first = m_count;
  }
  void scissor(dotz::vec2 pos, dotz::vec2 size) {
    run();

    pos = pos * m_bh;
    size = size * m_bh;

    vee::cmd_set_scissor(m_cb, {
      { static_cast<int>(pos.x), static_cast<int>(pos.y) },
      { static_cast<unsigned>(size.x), static_cast<unsigned>(size.y) },
    });
  }
};

class pipeline {
  voo::command_pool m_cpool {};
  vee::command_buffer m_cb = m_cpool.allocate_secondary_command_buffer();

  vee::pipeline_layout m_pl = vee::create_pipeline_layout(vee::vertex_push_constant_range<dotz::vec2>());

  vee::render_pass m_rp;
  vee::gr_pipeline m_gp;

  voo::bound_buffer m_buf;
  voo::one_quad m_quad;

public:
  pipeline(vee::physical_device pd, vee::surface::type surf) :
    m_rp {
      vee::create_render_pass({
        .attachments {{
          vee::create_passthru_colour_attachment(pd, surf),
        }},
        .subpasses {{
          vee::create_subpass({
            .colours {{ vee::create_attachment_ref(0, vee::image_layout_color_attachment_optimal) }},
          }),
        }},
        .dependencies {{
          vee::create_colour_dependency(),
        }},
      })
    }
  , m_gp {
    vee::create_graphics_pipeline({
      .pipeline_layout = *m_pl,
      .render_pass = *m_rp,
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
    })
  }
  , m_buf {
    voo::bound_buffer::create_from_host(pd, sizeof(inst) * max_inst, vee::buffer_usage::vertex_buffer)
  }
  , m_quad { pd }
  {}

  [[nodiscard]] constexpr auto command_buffer() const { return m_cb; }

  [[nodiscard]] auto record(vee::extent ext, float h) {
    float aspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);

    dotz::vec2 e { h * aspect, h };

    vee::begin_cmd_buf_render_pass_continue(m_cb, *m_rp);
    vee::cmd_set_viewport(m_cb, ext);
    vee::cmd_bind_gr_pipeline(m_cb, *m_gp);
    vee::cmd_push_vertex_constants(m_cb, *m_pl, &e);
    vee::cmd_bind_vertex_buffers(m_cb, 0, m_quad.buffer());
    vee::cmd_bind_vertex_buffers(m_cb, 1, *m_buf.buffer);

    return recorder { m_cb, ext.height / h, *m_buf.memory };
  }
};

vinyl::v<struct as, struct ss> g;
struct as {
  voo::device_and_queue dq { "poc", casein::native_ptr };

  vee::render_pass rp = voo::single_att_render_pass(dq);
};
struct ss {
  voo::swapchain sw { g.as()->dq };

  voo::command_pool cpool {};
  vee::command_buffer pcb = cpool.allocate_primary_command_buffer();

  hai::array<voo::swapchain::render_pass_pair> rpbs = sw.create_pairs(vee::render_pass_begin {
    .command_buffer = pcb,
    .render_pass = *g.as()->rp,
    .clear_colours { vee::clear_colour(0.01, 0.02, 0.03, 1.0) },
  });

  pipeline ppl {
    g.as()->dq.physical_device(),
    g.as()->dq.surface(),
  };

  bool recorded = false;
};

static void do_ui() {
  auto h = 10.f;
  auto w = g.ss()->sw.aspect() * h;

  auto r = g.ss()->ppl.record(g.ss()->sw.extent(), h);
  {
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

    for (auto i = 0; i < 20; i++) {
      auto scroll = 0.7f;
      dotz::vec2 p { 0.5f, 0.5f + i * 0.9f - scroll };
      r.push({
        .pos = p + dotz::vec2 { 0, 1 },
        .size { w - 1.f, 0.7f },
        .colour { 0.2f, 0.5f, 0.8f, 1.0f },
      });
    }

    r.run();
    r.scissor({ 0, 0 }, { w, h });

    for (auto i = 0; i < 5; i++) {
      dotz::vec2 p { i * w / 5.f, h - 1.f };
      dotz::vec2 s { w / 5.f, 1.f };
      r.push({
        .pos = p,
        .size = s,
        .colour { 0.5f },
      });
      r.push({
        .pos = p + 0.1f,
        .size = s - 0.2f,
        .colour { 0.1f, 0.2, 0.3f, 1.0f },
      });
    }
    r.run();
  }
}

static void on_frame() {
  if (!g.ss()->recorded) {
    do_ui();
    g.ss()->recorded = true;
  }

  auto pcb = g.ss()->pcb;

  voo::ots_present_guard pg { &g.ss()->sw, pcb };
  voo::cmd_render_pass rp { g.ss()->rpbs[g.ss()->sw.index()].rpb, false };
  vee::cmd_execute_command(pcb, g.ss()->ppl.command_buffer());
}

const int i = [] {
  g.on_frame() = on_frame;
  g.setup();
  return 0;
}();

