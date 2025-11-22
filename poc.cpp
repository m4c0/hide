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

  vee::render_pass srp = vee::create_render_pass({
    .attachments {{
      vee::create_passthru_colour_attachment(dq.physical_device(), dq.surface()),
    }},
    .subpasses {{
      vee::create_subpass({
        .colours {{ vee::create_attachment_ref(0, vee::image_layout_color_attachment_optimal) }},
      }),
    }},
    .dependencies {{
      vee::create_colour_dependency(),
    }},
  });

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

  voo::command_pool cpool {};
  vee::command_buffer pcb = cpool.allocate_primary_command_buffer();
  vee::command_buffer scb = cpool.allocate_secondary_command_buffer();

  hai::array<voo::swapchain::render_pass_pair> rpbs = sw.create_pairs(vee::render_pass_begin {
    .command_buffer = pcb,
    .render_pass = *g.as()->rp,
    .clear_colours { vee::clear_colour(0.01, 0.02, 0.03, 1.0) },
  });
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
  auto pcb = g.ss()->pcb;
  auto scb = g.ss()->scb;

  voo::ots_present_guard pg { &g.ss()->sw, pcb };
  voo::cmd_render_pass rp { g.ss()->rpbs[g.ss()->sw.index()].rpb, false };
  vee::cmd_execute_command(pcb, scb);
}

static void do_ui() {
  auto cb = g.ss()->scb;
  auto aspect = g.ss()->sw.aspect();
  auto ext = g.ss()->sw.extent();

  auto h = 10.f;
  auto w = aspect * h;
  upc pc {
    .extent { w, h },
  };

  voo::cmd_buf_render_pass_continue rpc { cb, *g.as()->srp };
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

  g.on_frame() = on_frame;
}

const int i = [] {
  g.on_frame() = do_ui;
  g.setup();
  return 0;
}();

