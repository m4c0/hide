#pragma leco app
#pragma leco add_shader "poc.frag"
#pragma leco add_shader "poc.vert"

import casein;
import dotz;
import hai;
import hide;
import no;
import vinyl;
import voo;

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

  hide::pipeline ppl {
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

