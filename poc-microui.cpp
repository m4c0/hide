#pragma leco app
#pragma leco add_resource "dungeon-437.png"

import casein;
import dotz;
import hai;
import hay;
import mu;
import print;
import sv;
import vinyl;
import voo;

void do_ui() {
  using namespace mu;

  auto ctx = block();
  auto wnd = window({ 10 }, { 380 });

  compos::layout_row({ 100, 100 }, 0);

  compos::text("One");
  if (compos::button("Hello")) putln("hello");

  compos::text("Two");
  if (compos::button("World")) putln("world");
}

struct as {
  voo::device_and_queue dq { "poc-microui", casein::native_ptr };
  vee::render_pass rp = vee::create_render_pass(vee::create_render_pass_params {
    .attachments {{
      vee::create_colour_attachment({
        .format = dq.find_best_surface_image_format(),
        .final_layout = vee::image_layout_attachment_optimal,
      })
    }},
    .subpasses {{
      vee::create_subpass({
        .colours {{ vee::create_attachment_ref(0, vee::image_layout_color_attachment_optimal) }},
      }),
    }},
    .dependencies {{ vee::create_colour_dependency() }},
  });
};
hai::uptr<as> gas {};

struct ss {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };

  mu::vulkan::pipeline ppl { gas->dq.physical_device(), {
    .format = gas->dq.find_best_surface_image_format(),
    .initial_layout = vee::image_layout_attachment_optimal,
    .final_layout = vee::image_layout_present_src_khr,
  }};
};
hai::uptr<ss> gss {};

static void on_frame() {
  if (!gss) gss.reset(new ss {});

  do_ui();

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [&] {
    {
      auto rp = gss->sw.cmd_render_pass({
        .clear_colours { vee::clear_colour(0.01f, 0.02f, 0.03f, 1.0f) },
      });
    }
    gss->ppl.render(gss->sw.render_pass_begin());
  });
  gss->sw.queue_present(gas->dq.queue());
}

const int i = [] {
  mu::font_name = [] { return "dungeon-437.png"_sv; };
  mu::text_height = [] { return 16; };

  using namespace vinyl;
  on(START, [] { gas.reset(new as {}); });
  on(STOP, [] { gss = {}; gas = {}; });
  on(FRAME, on_frame);
  on(RESIZE, [] { gss = {}; });
  return 0;
}();

