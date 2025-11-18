#pragma leco add_shader "hide.vert"
#pragma leco add_shader "hide.frag"

export module hide;
export import :microui;
import voo;

namespace hide::vulkan {
  export struct inst {
    dotz::vec2 pos;
    dotz::vec2 size;
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
    vee::pipeline_layout m_pl = vee::create_pipeline_layout(vee::vertex_push_constant_range<upc>());
    vee::render_pass m_rp;
    vee::gr_pipeline m_gp;

  public:
    pipeline(const vee::attachment_description & colour) :
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
        voo::shader { "poc.vert.spv" }.pipeline_vert_stage("main"),
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
    }) }
    {}

    void render(vee::command_buffer cb, vee::framebuffer::type fb, vee::extent ext,auto && fn) {
      voo::cmd_render_pass rp { vee::render_pass_begin {
        .command_buffer = cb,
        .render_pass = *m_rp,
        .framebuffer = fb,
        .extent = ext,
      }};
      vee::cmd_set_scissor(cb, ext);
      vee::cmd_set_viewport(cb, ext);
      vee::cmd_bind_gr_pipeline(cb, *m_gp);
      fn();
    }
  };
}
