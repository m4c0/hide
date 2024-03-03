#pragma leco app
#pragma leco add_shader "poc-imm.frag"
#pragma leco add_shader "poc-imm.vert"
#pragma leco add_resource "BrainF.png"
#pragma leco add_resource "Lenna_(test_image).png"
#pragma leco add_resource "m3-bg.png"
#pragma leco add_resource "m3-game_title.png"
#pragma leco add_resource "m3-storeCounter_bar.png"

#define SHOW_SPLASH 1

import casein;
import dotz;
import hide;
import jute;
import sitime;
import traits;
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

class pipeline {
  static constexpr const auto max_quads = 10240;

  voo::one_quad m_quad;
  voo::h2l_buffer m_insts;

  vee::command_pool m_cpool;
  vee::command_buffer m_scb;

  vee::render_pass::type m_rp;
  vee::descriptor_set_layout m_dsl;
  vee::pipeline_layout m_pl;
  vee::gr_pipeline m_gp;

public:
  pipeline(vee::physical_device pd, unsigned qf, vee::render_pass::type rp)
      : m_quad{pd}
      , m_insts{pd, max_quads * sizeof(inst)}
      , m_cpool{vee::create_command_pool(qf)}
      , m_scb{vee::allocate_secondary_command_buffer(*m_cpool)}
      , m_rp{rp}
      , m_dsl{vee::create_descriptor_set_layout({vee::dsl_fragment_sampler()})}
      , m_pl{vee::create_pipeline_layout(
            *m_dsl, vee::vertex_push_constant_range<upc>())}
      , m_gp{vee::create_graphics_pipeline({
            .pipeline_layout = *m_pl,
            .render_pass = rp,
            .depth_test = false,
            .shaders{
                voo::shader("poc-imm.vert.spv").pipeline_vert_stage(),
                voo::shader("poc-imm.frag.spv").pipeline_frag_stage(),
            },
            .bindings{
                m_quad.vertex_input_bind(),
                vee::vertex_input_bind_per_instance(sizeof(inst)),
            },
            .attributes{
                m_quad.vertex_attribute(0),
                vee::vertex_attribute_vec2(1, 0),
                vee::vertex_attribute_vec2(1, sizeof(dotz::vec2)),
                vee::vertex_attribute_vec2(1, 2 * sizeof(dotz::vec2)),
                vee::vertex_attribute_vec2(1, 3 * sizeof(dotz::vec2)),
                vee::vertex_attribute_vec4(1, 4 * sizeof(dotz::vec2)),
            },
        })} {}

  [[nodiscard]] constexpr const auto descriptor_set_layout() const noexcept {
    return *m_dsl;
  }
  [[nodiscard]] constexpr const auto host_memory() const noexcept {
    return m_insts.host_memory();
  }
  [[nodiscard]] constexpr const auto pipeline_layout() const noexcept {
    return *m_pl;
  }
  [[nodiscard]] constexpr const auto &one_quad() const noexcept {
    return m_quad;
  }
  [[nodiscard]] constexpr const auto secondary_command_buffer() const noexcept {
    return m_scb;
  }

  [[nodiscard]] auto cmd_buf_render_pass_continue(vee::extent ext) const {
    upc pc{static_cast<float>(ext.width) / static_cast<float>(ext.height)};

    voo::cmd_buf_render_pass_continue rpc{m_scb, m_rp};
    vee::cmd_set_viewport(*rpc, ext);
    vee::cmd_set_scissor(*rpc, ext);
    vee::cmd_bind_gr_pipeline(*rpc, *m_gp);
    vee::cmd_push_vertex_constants(*rpc, *m_pl, &pc);
    vee::cmd_bind_vertex_buffers(*rpc, 1, m_insts.local_buffer());
    return rpc;
  }

  void setup_copy(vee::command_buffer cb) { m_insts.setup_copy(cb); }
};
class ppl_render_pass {
  const pipeline *m_ppl;
  voo::cmd_buf_render_pass_continue m_rpc;
  voo::mapmem m_mem;
  inst *m_buf;
  inst *m_first;

public:
  ppl_render_pass(const pipeline *ppl, vee::extent ext)
      : m_ppl{ppl}
      , m_rpc{ppl->cmd_buf_render_pass_continue(ext)}
      , m_mem{ppl->host_memory()}
      , m_buf{static_cast<inst *>(*m_mem)}
      , m_first{m_buf} {}

  void run(vee::descriptor_set dset, auto &&fn) {
    auto base = m_buf;
    fn(m_buf);
    if (m_buf > base) {
      vee::cmd_bind_descriptor_set(*m_rpc, m_ppl->pipeline_layout(), 0, dset);
      m_ppl->one_quad().run(*m_rpc, 0, (m_buf - base), (base - m_first));
    }
  }

  void stamp(vee::descriptor_set dset, float y, dotz::vec2 sz, float a = 1.0f) {
    run(dset, [&](auto &buf) {
      *buf++ = {
          .r = {{-sz.x * 0.5f, y - sz.y * 0.5f}, sz},
          .uv = {{0, 0}, {1, 1}},
          .mult = {1.0f, 1.0f, 1.0f, a},
      };
    });
  }
};

class splash {
  hide::image m_img;
  float m_timer{};

public:
  explicit splash(hide::image img) : m_img{traits::move(img)} {}

  bool operator()(ppl_render_pass *rpc, float dt) {
    m_timer += dt;
    if (m_timer > 3000.0f)
      return false;

    float s = m_timer / 1000.0f;
    float a = sinf(s * 3.14 / 3.0);

    rpc->stamp(m_img.dset(), 0.0f, m_img.size(1.6f), a);
    // TODO: how to tween alpha and keep cmd_buf and vbuf intact?
    // TODO: lazy load image or pause until image is loaded?
    return true;
  }
};

class thread : public voo::casein_thread {
  volatile casein::keys m_last_key_down{};

  void key_down(const casein::events::key_down &e) override {
    m_last_key_down = *e;
  }
  void run() override {
    voo::device_and_queue dq{"hide-immediate", native_ptr()};

    pipeline ppl{dq.physical_device(), dq.queue_family(), dq.render_pass()};

    auto dsl = ppl.descriptor_set_layout();
    auto dpool =
        vee::create_descriptor_pool(16, {vee::combined_image_sampler(16)});
    const auto load_img = [&](jute::view fn) {
      return hide::image{dq.physical_device(), dq.queue(),
                         vee::allocate_descriptor_set(*dpool, dsl), fn};
    };

    splash spl1{load_img("BrainF.png")};
    splash spl2{load_img("Lenna_(test_image).png")};
    auto bg = load_img("m3-bg.png");
    auto logo = load_img("m3-game_title.png");
    auto bar_bg = load_img("m3-storeCounter_bar.png");

    hide::text mmtxt{dq.physical_device(), dq.queue(),
                     vee::allocate_descriptor_set(*dpool, dsl)};
    dotz::vec2 mmtxt_szs[]{
        mmtxt.draw("New Game"), mmtxt.draw("Continue"), mmtxt.draw("Options"),
        mmtxt.draw("Credits"),  mmtxt.draw("Exit"),
    };
    unsigned mmsel{};
    bool mmout{};
    float mmdt{};

    bool has_game{};

    sitime::stopwatch time{};

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      const auto refresh = [&] {
        float dt = time.millis();
        time = {};

        ppl_render_pass rpc{&ppl, sw.extent()};

        const auto main_menu = [&] {
          if (mmout && mmdt == 0.0f)
            return mmsel + 1;

          if (mmout) {
            mmdt -= dt;
            if (mmdt < 0)
              mmdt = 0.0f;
          } else {
            mmdt += dt;
            if (mmdt > 1000.0f)
              mmdt = 1000.0f;
          }

          float a = mmdt / 1000.0f;
          if (!mmout && a == 1.0f && m_last_key_down) {
            constexpr const auto mx = sizeof(mmtxt_szs) / sizeof(mmtxt_szs[0]);
            do {
              if (m_last_key_down == casein::K_DOWN)
                mmsel = (mmsel + 1) % mx;
              if (m_last_key_down == casein::K_UP)
                mmsel = (mx + mmsel - 1) % mx;
            } while (!has_game && mmsel == 1);

            if (m_last_key_down == casein::K_ENTER)
              mmout = true;
          }

          rpc.stamp(bg.dset(), 0.0f, {2.0f * sw.aspect(), 2.0f}, a);
          rpc.stamp(logo.dset(), -0.5f, logo.size(0.6f), a);

          const auto bar = [&](rect r) {
            constexpr const dotz::vec2 bsz{0.05f, -0.05f};
            constexpr const auto mgn = 0.05f;

            const auto xl = r.pos.x - bsz.x * 0.5f - mgn;
            const auto xm = xl + mgn;
            const auto xr = r.pos.x + r.size.x + bsz.x * 0.5f;
            const auto xw = xr - xm;

            const auto yt = r.pos.y - bsz.y * 0.5f - mgn;
            const auto ym = yt + mgn;
            const auto yb = r.pos.y + r.size.y + bsz.y * 0.5f;
            const auto yh = yb - ym;

            const dotz::vec4 m{1.0f, 1.0f, 1.0f, a};

            constexpr const auto ul = 0.25f;
            constexpr const auto ur = 1.0f - ul;
            constexpr const auto uw = ur - ul;
            constexpr const auto vt = 0.15f;
            constexpr const auto vb = 1.0f - vt;
            constexpr const auto vh = vb - vt;

            rpc.run(bar_bg.dset(), [&](auto &buf) {
              *buf++ = {.r = {{xl, yt}, {mgn}},
                        .uv = {{0.0f, 0.0f}, {ul, vt}},
                        .mult = m};
              *buf++ = {.r = {{xl, ym}, {mgn, yh}},
                        .uv = {{0.0f, vt}, {ul, vh}},
                        .mult = m};
              *buf++ = {.r = {{xl, yb}, {mgn}},
                        .uv = {{0.0f, vb}, {ul, vt}},
                        .mult = m};
              *buf++ = {.r = {{xm, yt}, {xw, mgn}},
                        .uv = {{ul, 0.0f}, {uw, vt}},
                        .mult = m};
              *buf++ = {.r = {{xm, ym}, {xw, yh}},
                        .uv = {{ul, vt}, {uw, vh}},
                        .mult = m};
              *buf++ = {.r = {{xm, yb}, {xw, mgn}},
                        .uv = {{ul, vb}, {uw, vt}},
                        .mult = m};
              *buf++ = {.r = {{xr, yt}, {mgn}},
                        .uv = {{ur, 0.0f}, {ul, vt}},
                        .mult = m};
              *buf++ = {.r = {{xr, ym}, {mgn, yh}},
                        .uv = {{ur, vt}, {ul, vh}},
                        .mult = m};
              *buf++ = {.r = {{xr, yb}, {mgn}},
                        .uv = {{ur, vb}, {ul, vt}},
                        .mult = m};
            });
          };
          {
            float y = 0.0f;
            unsigned i = 0;
            for (auto uv : mmtxt_szs) {
              auto sz = uv * 1.4f;
              auto hsz = -sz * 0.5f;
              if (i++ == mmsel) {
                bar({{hsz.x, y + hsz.y}, sz});
                break;
              }
              y += sz.y;
            }
          }

          rpc.run(mmtxt.dset(), [&](auto &buf) {
            float y = 0.0f;
            float v = 0.0f;
            unsigned i = 0;
            for (auto uv : mmtxt_szs) {
              float aa = (i == 1 && !has_game) ? 0.4f * a : a;
              auto sz = uv * 1.4f;
              auto hsz = -sz * 0.5f;
              auto colour = i++ == mmsel ? dotz::vec4{0.0f, 0.0f, 0.0f, aa}
                                         : dotz::vec4{0.5f, 0.2f, 0.1f, aa};
              *buf++ = {
                  .r = {{hsz.x, y + hsz.y}, sz},
                  .uv = {{0.0f, v}, uv},
                  .mult = colour,
              };
              y += sz.y;
              v += uv.y;
            }
          });
          return 0U;
        };

#if SHOW_SPLASH
        if (spl1(&rpc, dt))
          return;
        if (spl2(&rpc, dt))
          return;
#endif

        switch (main_menu()) {
        case 0:
          break;
        case 1: // new game
          has_game = true;
          mmout = false;
          break;
        case 2: // continue
          has_game = false;
          mmout = false;
          mmsel = 0;
          break;
        case 5:
          casein::exit(0);
          break;
        default:
          mmout = false;
          break;
        }
      };

      extent_loop(dq.queue(), sw, [&] {
        refresh();
        m_last_key_down = {};

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          mmtxt.setup_copy(*pcb);
          ppl.setup_copy(*pcb);

          auto rp = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_color = {},
              .use_secondary_cmd_buf = true,
          });
          vee::cmd_execute_command(*pcb, ppl.secondary_command_buffer());
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  t.handle(e);
}
