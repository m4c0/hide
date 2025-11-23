export module hide;
import dotz;
import no;
import traits;
import vee;
import voo;

namespace hide {
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

  export class pipeline {
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

}
