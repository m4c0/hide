#pragma leco app
#pragma leco add_impl microui
#pragma leco add_shader "poc.vert"
#pragma leco add_shader "poc.frag"

import casein;
import dotz;
import hai;
import hay;
import print;
import vinyl;
import voo;

extern "C" {
#include "microui/src/microui.h"
}

static auto init() {
  auto ctx = new mu_Context {};
  mu_init(ctx);
  return ctx;
}
static auto deinit(mu_Context * ptr) {
  delete ptr;
}

static int text_width(mu_Font, const char *, int len) {
  return 5 * len;
}
static int text_height(mu_Font) {
  return 8;
}

static auto ctx = [] {
  hay<mu_Context *, init, deinit> ctx {}; 
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  return ctx;
}();

void do_ui() {
  mu_begin(ctx);

  auto wnd_rect = mu_rect(10, 10, 380, 380);
  //auto wnd_opts = MU_OPT_NOCLOSE | MU_OPT_NOTITLE;
  auto wnd_opts = 0;
  if (mu_begin_window_ex(ctx, "Window", wnd_rect, wnd_opts)) {
    int cols[] { 100, 100 };
    mu_layout_row(ctx, 2, cols, 0);

    mu_text(ctx, "One");
    if (mu_button(ctx, "Hello")) putln("hello");

    mu_text(ctx, "Two");
    if (mu_button(ctx, "World")) putln("world");

    mu_end_window(ctx);
  }

  mu_end(ctx);
}

struct inst {
  dotz::vec2 pos;
  dotz::vec2 size;
  dotz::vec4 colour;
};

void draw_ui(voo::memiter<inst> * m) {
  mu_Command * cmd {};
  while (mu_next_command(ctx, &cmd)) {
    switch (cmd->type) {
      case MU_COMMAND_CLIP: {
        //auto [x, y, w, h] = cmd->clip.rect;
        //putan("clip", x, y, w, h);
        break;
      }
      case MU_COMMAND_ICON: {
        //auto [x, y, w, h] = cmd->icon.rect;
        //putan("icon", cmd->icon.id, x, y, w, h);
        break;
      }
      case MU_COMMAND_RECT: {
        auto [x, y, w, h] = cmd->rect.rect;
        auto [r, g, b, a] = cmd->rect.color;
        *m += inst {
          .pos { x, y },
          .size { w, h },
          .colour = dotz::vec4 { r, g, b, a } / 255.0,
        };
        break;
      }
      case MU_COMMAND_TEXT:
        //putan("text", cmd->text.pos.x, cmd->text.pos.y, cmd->text.str);
        break;
      default: break;
    }
  }
}

struct as {
  static constexpr const auto max_inst = 1024;

  voo::device_and_queue dq { "poc-microui", casein::native_ptr };
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
      voo::shader { "poc.vert.spv" }.pipeline_vert_stage("main", vee::specialisation_info<float>(0, sw.aspect())),
      voo::shader { "poc.frag.spv" }.pipeline_frag_stage("main"),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
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
hai::uptr<ss> gss {};

static unsigned map() {
  unsigned count = 0;
  voo::memiter<inst> m { *gas->buf.memory, &count };
  do_ui();
  draw_ui(&m);
  return count;
}

static void on_frame() {
  if (!gss) gss.reset(new ss {});

  auto count = map();

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

