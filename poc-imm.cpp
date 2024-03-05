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
import hai;
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
  static constexpr const auto max_dsets = 16;
  static constexpr const auto max_quads = 10240;

  voo::one_quad m_quad;
  voo::h2l_buffer m_insts;

  vee::command_pool m_cpool;
  vee::command_buffer m_scb;

  vee::render_pass::type m_rp;
  vee::descriptor_set_layout m_dsl;
  vee::descriptor_pool m_dpool;
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
      , m_dpool{vee::create_descriptor_pool(
            max_dsets, {vee::combined_image_sampler(max_dsets)})}
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

  [[nodiscard]] auto allocate_descriptor_set() {
    return vee::allocate_descriptor_set(*m_dpool, *m_dsl);
  }

  void setup_copy(vee::command_buffer cb) { m_insts.setup_copy(cb); }
};
class ppl_render_pass {
  const pipeline *m_ppl;
  voo::cmd_buf_render_pass_continue m_rpc;
  voo::mapmem m_mem;
  inst *m_buf;
  inst *m_first;
  inst *m_base;

  void end_cycle() {
    if (m_buf > m_base) {
      m_ppl->one_quad().run(*m_rpc, 0, (m_buf - m_base), (m_base - m_first));
    }
    m_base = m_buf;
  }

public:
  ppl_render_pass(const pipeline *ppl, vee::extent ext)
      : m_ppl{ppl}
      , m_rpc{ppl->cmd_buf_render_pass_continue(ext)}
      , m_mem{ppl->host_memory()}
      , m_buf{static_cast<inst *>(*m_mem)}
      , m_first{m_buf}
      , m_base{m_buf} {}
  ~ppl_render_pass() { end_cycle(); }

  void bind_descriptor_set(vee::descriptor_set dset) {
    end_cycle();
    vee::cmd_bind_descriptor_set(*m_rpc, m_ppl->pipeline_layout(), 0, dset);
  }
  void push_instance(const inst &i) { *m_buf++ = i; }

  void stamp(vee::descriptor_set dset, float y, dotz::vec2 sz, rect uv,
             float a) {
    bind_descriptor_set(dset);
    push_instance(inst{
        .r = {{-sz.x * 0.5f, y - sz.y * 0.5f}, sz},
        .uv = uv,
        .mult = {1.0f, 1.0f, 1.0f, a},
    });
  }
};

class selection_bar {
  hide::image m_img;

public:
  explicit selection_bar(hide::image img) : m_img{traits::move(img)} {}

  void operator()(ppl_render_pass *rpc, rect r, float a) {
    rpc->bind_descriptor_set(m_img.dset());

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

    rpc->push_instance(inst{
        .r = {{xl, yt}, {mgn}}, .uv = {{0.0f, 0.0f}, {ul, vt}}, .mult = m});
    rpc->push_instance(inst{
        .r = {{xl, ym}, {mgn, yh}}, .uv = {{0.0f, vt}, {ul, vh}}, .mult = m});
    rpc->push_instance(
        inst{.r = {{xl, yb}, {mgn}}, .uv = {{0.0f, vb}, {ul, vt}}, .mult = m});
    rpc->push_instance(inst{
        .r = {{xm, yt}, {xw, mgn}}, .uv = {{ul, 0.0f}, {uw, vt}}, .mult = m});
    rpc->push_instance(
        inst{.r = {{xm, ym}, {xw, yh}}, .uv = {{ul, vt}, {uw, vh}}, .mult = m});
    rpc->push_instance(inst{
        .r = {{xm, yb}, {xw, mgn}}, .uv = {{ul, vb}, {uw, vt}}, .mult = m});
    rpc->push_instance(
        inst{.r = {{xr, yt}, {mgn}}, .uv = {{ur, 0.0f}, {ul, vt}}, .mult = m});
    rpc->push_instance(inst{
        .r = {{xr, ym}, {mgn, yh}}, .uv = {{ur, vt}, {ul, vh}}, .mult = m});
    rpc->push_instance(
        inst{.r = {{xr, yb}, {mgn}}, .uv = {{ur, vb}, {ul, vt}}, .mult = m});
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

    rpc->stamp(m_img.dset(), 0.0f, m_img.size(1.6f), {{0, 0}, {1, 1}}, a);
    // TODO: how to tween alpha and keep cmd_buf and vbuf intact?
    // TODO: lazy load image or pause until image is loaded?
    return true;
  }
};

class menu_option {
  static unsigned new_id() {
    static unsigned i{};
    return ++i;
  }

  [[maybe_unused]] hide::text *m_txt;
  dotz::vec4 m_uv;
  unsigned m_id{new_id()};

public:
  menu_option(hide::text *t, jute::view str) : m_txt{t}, m_uv{t->draw(str)} {}

  bool operator()(ppl_render_pass *rpc, float y, unsigned sel, float a) {
    dotz::vec2 sz{m_uv.z, m_uv.w};
    rect uv{{m_uv.x, m_uv.y}, {m_uv.z, m_uv.w}};
    rpc->stamp(m_txt->dset(), y, sz * 1.4f, uv, a);
    return sel == m_id;
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

    const auto load_img = [&](jute::view fn) {
      return hide::image{dq.physical_device(), dq.queue(),
                         ppl.allocate_descriptor_set(), fn};
    };

    splash spl1{load_img("BrainF.png")};
    splash spl2{load_img("Lenna_(test_image).png")};
    auto bg = load_img("m3-bg.png");
    auto logo = load_img("m3-game_title.png");
    selection_bar bar{load_img("m3-storeCounter_bar.png")};

    hide::text mmtxt{dq.physical_device(), dq.queue(),
                     ppl.allocate_descriptor_set()};

    menu_option mm_new{&mmtxt, "New Game"};
    menu_option mm_cont{&mmtxt, "Continue"};
    menu_option mm_opts{&mmtxt, "Options"};
    menu_option mm_creds{&mmtxt, "Credits"};
    menu_option mm_exit{&mmtxt, "Exit"};

    mmtxt.run_once();

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
          constexpr const auto fade_duration = 300.0f;
          if (mmout && mmdt == 0.0f)
            return mmsel + 1;

          if (mmout) {
            mmdt -= dt;
            if (mmdt < 0)
              mmdt = 0.0f;
          } else {
            mmdt += dt;
            if (mmdt > fade_duration)
              mmdt = fade_duration;
          }

          float a = mmdt / fade_duration;
          if (!mmout && a == 1.0f && m_last_key_down) {
            /*
            const auto mx = mmtxt_szs.size();
            do {
              if (m_last_key_down == casein::K_DOWN)
                mmsel = (mmsel + 1) % mx;
              if (m_last_key_down == casein::K_UP)
                mmsel = (mx + mmsel - 1) % mx;
            } while (!has_game && mmsel == 1);

            if (m_last_key_down == casein::K_ENTER)
              mmout = true;
              */
          }

          rpc.stamp(bg.dset(), 0.0f, {2.0f * sw.aspect(), 2.0f},
                    {{0, 0}, {1, 1}}, a);
          rpc.stamp(logo.dset(), -0.5f, logo.size(0.6f), {{0, 0}, {1, 1}}, a);
          // bar(&rpc, mmtxt_is[mmsel].r, a);

          mm_new(&rpc, 0.0f, mmsel, a);
          mm_cont(&rpc, 0.1f, mmsel, a);
          mm_opts(&rpc, 0.2f, mmsel, a);
          mm_creds(&rpc, 0.3f, mmsel, a);
          mm_exit(&rpc, 0.4f, mmsel, a);
          /*
          rpc.run(mmtxt.dset(), [&](auto &buf) {
            for (auto i = 0; i < mmtxt_szs.size(); i++) {
              float aa = (i == 1 && !has_game) ? 0.4f * a : a;

              auto inst = mmtxt_is[i];
              inst.mult = i == mmsel ? dotz::vec4{0.0f, 0.0f, 0.0f, aa}
                                     : dotz::vec4{0.5f, 0.2f, 0.1f, aa};

              *buf++ = inst;
            }
          });
          */
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
