#pragma leco app
#pragma leco add_shader "poc-imm.frag"
#pragma leco add_shader "poc-imm.vert"

import casein;
import dotz;
import vee;
import voo;

struct rect {
  dotz::vec2 pos;
  dotz::vec2 size;
};
struct upc {
  float aspect;
};

static constexpr const auto max_quads = 10240;

class thread : public voo::casein_thread {
  void run() override {
    voo::device_and_queue dq{"hide-immediate", native_ptr()};

    voo::one_quad quad{dq.physical_device()};
    voo::h2l_buffer insts{dq.physical_device(), max_quads * sizeof(rect)};

    {
      voo::mapmem m{insts.host_memory()};
      auto buf = static_cast<rect *>(*m);
      *buf++ = {{-0.8f, -0.8f}, {1.6f, 1.6f}};
    }

    auto pl =
        vee::create_pipeline_layout({vee::vertex_push_constant_range<upc>()});
    auto gp = vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = dq.render_pass(),
        .shaders{
            voo::shader("poc-imm.vert.spv").pipeline_vert_stage(),
            voo::shader("poc-imm.frag.spv").pipeline_frag_stage(),
        },
        .bindings{
            quad.vertex_input_bind(),
            vee::vertex_input_bind_per_instance(sizeof(rect)),
        },
        .attributes{
            quad.vertex_attribute(0),
            vee::vertex_attribute_vec2(1, 0),
            vee::vertex_attribute_vec2(1, sizeof(dotz::vec2)),
        },
    });

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      extent_loop(dq.queue(), sw, [&] {
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          insts.setup_copy(*pcb);

          upc pc{sw.aspect()};

          auto rp = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_color = {},
          });
          vee::cmd_bind_gr_pipeline(*pcb, *gp);
          vee::cmd_push_vertex_constants(*pcb, *pl, &pc);
          vee::cmd_bind_vertex_buffers(*pcb, 1, insts.local_buffer());
          quad.run(*pcb, 0);
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  t.handle(e);
}
