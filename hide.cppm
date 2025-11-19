#pragma leco add_shader "hide.vert"
#pragma leco add_shader "hide.frag"

export module hide;
export import :microui;
import silog;
import voo;

namespace hide::vulkan {
  export struct inst {
    dotz::vec2 pos;
    dotz::vec2 size;
    dotz::vec2 uv {};
    dotz::vec4 colour;
  };

  struct upc {
    float aspect;
  };

  static auto colour_attachment(vee::attachment_description c) {
    c.load_op = vee::attachment_load_op_load;
    return vee::create_colour_attachment(c);
  }

  export class pipeline {
    static constexpr const auto max_inst = 1024;

    voo::single_frag_dset m_dset { 1 };

    vee::pipeline_layout m_pl = vee::create_pipeline_layout(m_dset.descriptor_set_layout(), vee::vertex_push_constant_range<upc>());
    vee::render_pass m_rp;
    vee::gr_pipeline m_gp;

    voo::bound_buffer m_buf;
    voo::one_quad m_quad;

    voo::bound_image m_img {};
    vee::sampler m_smp = vee::create_sampler(vee::nearest_sampler);

  public:
    pipeline(vee::physical_device pd, const vee::attachment_description & colour) :
      m_rp { vee::create_render_pass(vee::create_render_pass_params {
        .attachments {{ colour_attachment(colour) }},
        .subpasses {{
          vee::create_subpass({
            .colours {{ vee::create_attachment_ref(0, vee::image_layout_color_attachment_optimal) }},
          }),
        }},
        .dependencies {{ vee::create_colour_dependency() }},
      }) }
    , m_gp { vee::create_graphics_pipeline({
      .pipeline_layout = *m_pl,
      .render_pass = *m_rp,
      .shaders {
        voo::shader { "hide.vert.spv" }.pipeline_vert_stage("main"),
        voo::shader { "hide.frag.spv" }.pipeline_frag_stage("main"),
      },
      .bindings {
        vee::vertex_input_bind(sizeof(dotz::vec2)),
        vee::vertex_input_bind_per_instance(sizeof(inst)),
      },
      .attributes {
        vee::vertex_attribute_vec2(0, 0),
        vee::vertex_attribute_vec2(1, traits::offset_of(&inst::pos)),
        vee::vertex_attribute_vec2(1, traits::offset_of(&inst::size)),
        vee::vertex_attribute_vec2(1, traits::offset_of(&inst::uv)),
        vee::vertex_attribute_vec4(1, traits::offset_of(&inst::colour)),
      },
    }) }
    , m_buf { voo::bound_buffer::create_from_host(pd, max_inst * sizeof(hide::vulkan::inst), vee::buffer_usage::vertex_buffer) }
    , m_quad { pd }
    {
      voo::load_image(font_name(), pd, voo::queue::instance(), &m_img, [this](auto sz) {
        vee::update_descriptor_set(m_dset.descriptor_set(), 0, *m_img.iv, *m_smp);
      });
    }

    void render(vee::render_pass_begin rpb) {
      auto cb = rpb.command_buffer;
      rpb.render_pass = *m_rp;

      upc pc {
        .aspect = static_cast<float>(rpb.extent.width) / static_cast<float>(rpb.extent.height)
      };

      voo::cmd_render_pass rp { rpb };
      vee::cmd_set_scissor(cb, rpb.extent);
      vee::cmd_set_viewport(cb, rpb.extent);
      vee::cmd_bind_gr_pipeline(cb, *m_gp);
      vee::cmd_bind_vertex_buffers(cb, 1, *m_buf.buffer);
      vee::cmd_bind_descriptor_set(cb, *m_pl, 0, m_dset.descriptor_set());
      vee::cmd_push_vertex_constants(cb, *m_pl, &pc);

      unsigned count = 0;
      voo::memiter<hide::vulkan::inst> m { *m_buf.memory, &count };
      hide::for_each_command(
          [&](hide::commands::clip cmd) {
            static bool logged = false;
            if (logged) return;
            silog::log(silog::info, "clipped !!!!!!!!!!!!!!!!!!!!!!!");
            logged = true;
          },
          [&](hide::commands::icon cmd) {
            auto [x, y, w, h] = cmd.rect;
            auto [r, g, b, a] = cmd.color;
            // TODO: actually draw the icon
            m += hide::vulkan::inst {
              .pos { x, y },
              .size { w, h },
              .colour = dotz::vec4 { r, g, b, a } / 255.0,
            };
          },
          [&](hide::commands::rect cmd) {
            auto [x, y, w, h] = cmd.rect;
            auto [r, g, b, a] = cmd.color;
            m += hide::vulkan::inst {
              .pos { x, y },
              .size { w, h },
              .colour = dotz::vec4 { r, g, b, a } / 255.0,
            };
          },
          [&](const hide::commands::text & cmd) {
            auto [x, y] = cmd.pos;
            auto [r, g, b, a] = cmd.color;
            auto h = text_height();
            for (const auto * p = cmd.str; *p; p++) {
              float u = static_cast<float>(*p % 16) / 16;
              float v = static_cast<float>(*p / 16) / 16;
              m += hide::vulkan::inst {
                .pos { x, y },
                .size { h },
                .uv { u, v },
                .colour = dotz::vec4 { r, g, b, a } / 255.0,
              };
              x += h;
            }
          });

      m_quad.run(cb, 0, count);
    }
  };
}
