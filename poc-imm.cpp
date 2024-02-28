#pragma leco app
#pragma leco add_shader "poc-imm.frag"
#pragma leco add_shader "poc-imm.vert"
#pragma leco add_resource "BrainF.png"
#pragma leco add_resource "Lenna_(test_image).png"
#pragma leco add_resource "m3-bg.png"
#pragma leco add_resource "m3-game_title.png"

import casein;
import dotz;
import hide;
import sitime;
import vee;
import voo;

extern "C" float sinf(float);

struct rect {
  dotz::vec2 pos;
  dotz::vec2 size;
};
struct inst {
  rect r;
  rect uv;
  dotz::vec4 mult;
};
struct upc {
  float aspect;
};

static constexpr const auto max_quads = 10240;

class thread : public voo::casein_thread {
  void run() override {
    voo::device_and_queue dq{"hide-immediate", native_ptr()};

    voo::one_quad quad{dq.physical_device()};
    voo::h2l_buffer insts{dq.physical_device(), max_quads * sizeof(inst)};

    auto cpool = vee::create_command_pool(dq.queue_family());
    auto scb = vee::allocate_secondary_command_buffer(*cpool);

    auto dpool =
        vee::create_descriptor_pool(16, {vee::combined_image_sampler(16)});
    auto dsl = vee::create_descriptor_set_layout({vee::dsl_fragment_sampler()});

    hide::image spl1{dq.physical_device(), dq.queue(),
                     vee::allocate_descriptor_set(*dpool, *dsl), "BrainF.png"};
    hide::image spl2{dq.physical_device(), dq.queue(),
                     vee::allocate_descriptor_set(*dpool, *dsl),
                     "Lenna_(test_image).png"};
    hide::image bg{dq.physical_device(), dq.queue(),
                   vee::allocate_descriptor_set(*dpool, *dsl), "m3-bg.png"};
    hide::image logo{dq.physical_device(), dq.queue(),
                     vee::allocate_descriptor_set(*dpool, *dsl),
                     "m3-game_title.png"};

    hide::text main_menu{dq.physical_device(), dq.queue(),
                         vee::allocate_descriptor_set(*dpool, *dsl)};
    dotz::vec2 main_menu_szs[]{
        main_menu.draw("New Game"), main_menu.draw("Continue"),
        main_menu.draw("Options"),  main_menu.draw("Credits"),
        main_menu.draw("Exit"),
    };

    auto pl = vee::create_pipeline_layout(
        {*dsl}, {vee::vertex_push_constant_range<upc>()});
    auto gp = vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = dq.render_pass(),
        .depth_test = false,
        .shaders{
            voo::shader("poc-imm.vert.spv").pipeline_vert_stage(),
            voo::shader("poc-imm.frag.spv").pipeline_frag_stage(),
        },
        .bindings{
            quad.vertex_input_bind(),
            vee::vertex_input_bind_per_instance(sizeof(inst)),
        },
        .attributes{
            quad.vertex_attribute(0),
            vee::vertex_attribute_vec2(1, 0),
            vee::vertex_attribute_vec2(1, sizeof(dotz::vec2)),
            vee::vertex_attribute_vec2(1, 2 * sizeof(dotz::vec2)),
            vee::vertex_attribute_vec2(1, 3 * sizeof(dotz::vec2)),
            vee::vertex_attribute_vec4(1, 4 * sizeof(dotz::vec2)),
        },
    });

    sitime::stopwatch time{};

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      const auto refresh = [&] {
        upc pc{sw.aspect()};

        auto rpc = sw.cmd_buf_render_pass_continue(scb);
        vee::cmd_bind_gr_pipeline(*rpc, *gp);
        vee::cmd_push_vertex_constants(*rpc, *pl, &pc);
        vee::cmd_bind_vertex_buffers(*rpc, 1, insts.local_buffer());

        voo::mapmem m{insts.host_memory()};
        auto buf = static_cast<inst *>(*m);
        const auto first = buf;

        const auto stamp = [&](auto &img, float y, dotz::vec2 sz,
                               float a = 1.0f) {
          auto base = buf;
          vee::cmd_bind_descriptor_set(*rpc, *pl, 0, img.dset());
          *buf++ = {
              .r = {{-sz.x * 0.5f, y - sz.y * 0.5f}, sz},
              .uv = {{0, 0}, {1, 1}},
              .mult = {1.0f, 1.0f, 1.0f, a},
          };
          quad.run(*rpc, 0, (buf - base), (base - first));
        };
        const auto splash = [&](auto &spl, float ms) {
          if (ms > 3000.0f)
            return false;

          float s = ms / 1000.0f;
          float a = sinf(s * 3.14 / 3.0);

          stamp(spl, 0.0f, spl.size(1.6f), a);
          // TODO: how to tween alpha and keep cmd_buf and vbuf intact?
          // TODO: lazy load image or pause until image is loaded?
          return true;
        };

        if (splash(spl1, time.millis()))
          return;
        if (splash(spl2, time.millis() - 3000.0f))
          return;

        // main menu
        stamp(bg, 0.0f, {2.0f * sw.aspect(), 2.0f});
        stamp(logo, -0.5f, logo.size(0.6f));

        auto base = buf;
        vee::cmd_bind_descriptor_set(*rpc, *pl, 0, main_menu.dset());
        float y = 0.0f;
        float v = 0.0f;
        for (auto uv : main_menu_szs) {
          auto sz = uv * 1.4f;
          auto hsz = -sz * 0.5f;
          *buf++ = {
              .r = {{hsz.x, y + hsz.y}, sz},
              .uv = {{0.0f, v}, uv},
              .mult = {0.5f, 0.2f, 0.1f, 1.0f},
          };
          y += sz.y;
          v += uv.y;
        }
        quad.run(*rpc, 0, (buf - base), (base - first));

        // selection
        // menu
      };

      extent_loop(dq.queue(), sw, [&] {
        refresh();

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          main_menu.setup_copy(*pcb);
          insts.setup_copy(*pcb);

          auto rp = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_color = {},
              .use_secondary_cmd_buf = true,
          });
          vee::cmd_execute_command(*pcb, scb);
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  t.handle(e);
}
