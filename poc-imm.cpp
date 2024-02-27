#pragma leco app
#pragma leco add_shader "poc-imm.frag"
#pragma leco add_shader "poc-imm.vert"
#pragma leco add_resource "BrainF.png"

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

    voo::sires_image spl1{"BrainF.png", &dq};
    spl1.run_once();
    auto spl1_aspect =
        static_cast<float>(spl1.width()) / static_cast<float>(spl1.height());

    {
      voo::mapmem m{insts.host_memory()};
      auto buf = static_cast<rect *>(*m);
      buf[0] = {{-0.8f * spl1_aspect, -0.8f}, {1.6f * spl1_aspect, 1.6f}};
    }

    auto dsl = vee::create_descriptor_set_layout({vee::dsl_fragment_sampler()});
    auto pl = vee::create_pipeline_layout(
        {*dsl}, {vee::vertex_push_constant_range<upc>()});
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

    auto dpool =
        vee::create_descriptor_pool(1, {vee::combined_image_sampler()});
    auto dset = vee::allocate_descriptor_set(*dpool, *dsl);

    auto smp = vee::create_sampler(vee::linear_sampler);

    vee::update_descriptor_set(dset, 0, spl1.iv(), *smp);

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
          vee::cmd_bind_descriptor_set(*pcb, *pl, 0, dset);
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
