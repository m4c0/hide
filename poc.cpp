#pragma leco app
#pragma leco add_resource "BrainF.png"
#pragma leco add_resource "Lenna_(test_image).png"
#pragma leco add_resource "m3-bg.png"
#pragma leco add_resource "m3-game_title.png"
#pragma leco add_resource "m3-storeCounter_bar.png"
#pragma leco add_resource "VictorMono-Regular.otf"
export module poc;

import casein;
import hai;
import jute;
import silog;
import sitime;
import quack;
import sith;
import traits;
import vee;
import voo;
import what_the_font;

// Desired workflow:
// * Splash screen (fade in/out)
// * Second splash screen (fade in/out)
// * Menu with three options (tween in/out):
//   1. Start game
//   2. Options
//   3. Credits
//   4. Exit (available only on Win/Mac)
// * Start game = blank screen for 2 seconds
// * Options = a list of tabs
//   - List TBD, but it must contain "fullscreen" and "audio volume"
// * Credits = just "author" and some OSS credits (tween in/out)
// * Exit = duh

static wtf::library g_wtf{};

class scene : public voo::update_thread {
public:
  using update_thread::run;
  using update_thread::update_thread;

  virtual ~scene() = default;

  virtual scene *next() = 0;
  virtual void run(voo::swapchain_and_stuff *sw,
                   const voo::cmd_buf_one_time_submit &pcb) = 0;

  virtual void key_down(casein::keys k) {}
};

class image {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::sires_image m_img;
  vee::descriptor_set m_dset;

public:
  image(voo::device_and_queue *dq, quack::pipeline_stuff *ps, jute::view name)
      : m_img{name, dq}
      , m_dset{ps->allocate_descriptor_set(m_img.iv(), *m_smp)} {
    m_img.run_once();
  }

  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }
  [[nodiscard]] constexpr auto aspect() const noexcept {
    return static_cast<float>(m_img.width()) /
           static_cast<float>(m_img.height());
  }

  [[nodiscard]] constexpr auto size(float h) const noexcept {
    return quack::size{h * aspect(), h};
  }
};
class texts : public voo::update_thread {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::h2l_image m_img;
  vee::descriptor_set m_dset;

  void build_cmd_buf(vee::command_buffer cb) override {
    voo::cmd_buf_one_time_submit pcb{cb};
    m_img.setup_copy(cb);
  }

public:
  texts(voo::device_and_queue *dq, quack::pipeline_stuff *ps)
      : update_thread{dq}
      , m_img{*dq, 1024, 1024, false}
      , m_dset{ps->allocate_descriptor_set(m_img.iv(), *m_smp)} {
    constexpr const auto line_h = 128;
    constexpr const auto font_h = 100;
    wtf::face f = g_wtf.new_face("VictorMono-Regular.otf", font_h);
    voo::mapmem m{m_img.host_memory()};
    auto img = static_cast<unsigned char *>(*m);
    f.shape_pt("New Game").draw(img, 1024, 1024, 0, font_h);
    f.shape_pt("Continue").draw(img, 1024, 1024, 0, font_h + line_h);
    f.shape_pt("Options").draw(img, 1024, 1024, 0, font_h + line_h * 2);
    f.shape_pt("Credits").draw(img, 1024, 1024, 0, font_h + line_h * 3);
    f.shape_pt("Exit").draw(img, 1024, 1024, 0, font_h + line_h * 4);
  }

  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }

  using update_thread::run_once;
};

class splash : public scene {
  voo::device_and_queue *m_dq;

  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  image m_img;

  sitime::stopwatch m_time{};

  [[nodiscard]] auto time() const noexcept { return m_time.millis() / 1000.0; }
  [[nodiscard]] auto alpha() const noexcept {
    float t = time();
    float a = 1.0;
    if (t < 1) {
      a = t;
    } else if (t > 2) {
      a = 3.0 - t;
      if (a < 0)
        a = 0;
    }
    return a;
  }

  void build_cmd_buf(vee::command_buffer cb) override {
    auto a = alpha();
    m_ib.map_multipliers([a](auto *ms) { *ms = {1, 1, 1, a}; });

    voo::cmd_buf_one_time_submit pcb{cb};
    m_ib.setup_copy(cb);
  }

  using update_thread::run;

protected:
  [[nodiscard]] constexpr auto *device_and_queue() const noexcept {
    return m_dq;
  }

  virtual scene *create_next() = 0;

public:
  splash(voo::device_and_queue *dq, jute::view name)
      : scene{dq}
      , m_dq{dq}
      , m_ps{*dq, 1}
      , m_ib{m_ps.create_batch(1)}
      , m_img{dq, &m_ps, name} {
    m_ib.map_all([this](auto all) {
      auto img_aspect = m_img.aspect();
      all.positions[0] = {{-img_aspect / 2.f, 0}, {img_aspect, 1}};
      all.multipliers[0] = {1, 1, 1, 1};
      all.colours[0] = {0, 0, 0, 1};
      all.uvs[0] = {{0, 0}, {1, 1}};
    });
  }

  [[nodiscard]] scene *next() override {
    if (time() > 3)
      return create_next();
    return this;
  }

  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) override {
    auto pc = quack::adjust_aspect(
        {
            .grid_pos = {0.0f, 0.5f},
            .grid_size = {1.0f, 1.0f},
        },
        sw->aspect());

    auto rp = sw->cmd_render_pass({
        .command_buffer = *pcb,
        .clear_color = {{0, 0, 0, 1}},
    });
    m_ib.build_commands(*pcb);
    m_ps.cmd_push_vert_frag_constants(*pcb, pc);
    m_ps.cmd_bind_descriptor_set(*pcb, m_img.dset());
    m_ps.run(*pcb, 1);
  }
};

class main_menu : public scene {
  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  image m_bg;
  image m_logo;
  image m_sel;
  texts m_texts;
  sitime::stopwatch m_time{};

  unsigned m_idx{};

  static constexpr const auto max_dset = 4;
  static constexpr const auto max_sprites = 2 + 5 + 9;
  static constexpr const auto sel_border = 0.01f;

  void build_cmd_buf(vee::command_buffer cb) override {
    auto a = alpha();
    m_ib.map_multipliers([this, a](auto *ms) {
      for (auto i = 0; i < max_sprites; i++)
        ms[i] = {1, 1, 1, a};

      for (auto i = 2; i < 7; i++) {
        float n = (m_idx == i - 2) ? 0 : 1;
        ms[i] = {n, 0, 0, a};
      }
    });

    m_ib.map_positions([this](auto *ps) {
      auto p = ps[m_idx + 2];
      ps[7] = p;

      ps[8].x = ps[9].x = ps[10].x = p.x - sel_border;
      ps[14].x = ps[15].x = p.x;
      ps[11].x = ps[12].x = ps[13].x = p.x + p.w;

      ps[8].y = ps[11].y = ps[14].y = p.y - sel_border;
      ps[9].y = ps[12].y = p.y;
      ps[10].y = ps[13].y = ps[15].y = p.y + p.h;
    });

    voo::cmd_buf_one_time_submit pcb{cb};
    m_ib.setup_copy(cb);
  }

  [[nodiscard]] auto time() const noexcept { return m_time.millis() / 1000.0; }
  [[nodiscard]] float alpha() const noexcept {
    float t = time();
    return t < 1 ? t : 1.0f;
  }

  using update_thread::run;

public:
  explicit main_menu(voo::device_and_queue *dq)
      : scene{dq}
      , m_ps{*dq, max_dset}
      , m_ib{m_ps.create_batch(max_sprites)}
      , m_bg{dq, &m_ps, "m3-bg.png"}
      , m_logo{dq, &m_ps, "m3-game_title.png"}
      , m_sel{dq, &m_ps, "m3-storeCounter_bar.png"}
      , m_texts{dq, &m_ps} {
    m_ib.map_all([](auto all) {
      for (auto i = 0; i < max_sprites; i++) {
        all.multipliers[i] = {1, 1, 1, 1};
        all.colours[i] = {0, 0, 0, 0};
      }

      all.uvs[0] = {{0, 0}, {1, 1}};
      all.uvs[1] = {{0, 0}, {1, 1}};
      for (auto i = 0; i < 5; i++) {
        all.uvs[2 + i] = {{0.0f, i * 0.125f}, {0.45f, (i + 1) * 0.125f}};
      }

      all.uvs[7] = {{0.25f, 0.25f}, {0.75f, 0.75f}};
      all.uvs[8] = {{0.00f, 0.00f}, {0.25f, 0.25f}};
      all.uvs[9] = {{0.00f, 0.25f}, {0.25f, 0.75f}};
      all.uvs[10] = {{0.00f, 0.75f}, {0.25f, 1.00f}};
      all.uvs[11] = {{0.75f, 0.00f}, {1.00f, 0.25f}};
      all.uvs[12] = {{0.75f, 0.25f}, {1.00f, 0.75f}};
      all.uvs[13] = {{0.75f, 0.75f}, {1.00f, 1.00f}};
      all.uvs[14] = {{0.25f, 0.00f}, {0.75f, 0.25f}};
      all.uvs[15] = {{0.25f, 0.75f}, {0.75f, 1.00f}};
    });

    m_ib.map_positions([this](auto *ps) {
      constexpr const auto menu_w = 0.25f;
      constexpr const auto menu_h = 0.0625f;

      ps[0] = {{-2.f, -2.f}, {4.f, 4.f}};

      ps[1] = {{0, 0}, m_logo.size(0.5f)};
      for (auto i = 0; i < 5; i++) {
        ps[2 + i] = {{0, 0.05f + 0.5f + i * 0.0625f}, {menu_w, menu_h}};
      }

      auto h = 1.f - 0.05f;
      for (auto i = 1; i < 7; i++) {
        ps[i].x = -ps[i].w / 2.0f;
        h -= ps[i].h;
      }
      for (auto i = 1; i < 7; i++) {
        ps[i].y += h / 2.0f;
      }

      ps[7] = {};
      ps[8] = ps[10] = ps[11] = ps[13] = {{}, {sel_border, sel_border}};
      ps[9] = ps[12] = {{}, {sel_border, menu_h}};
      ps[14] = ps[15] = {{}, {menu_w, sel_border}};
    });

    m_texts.run_once();
  }

  scene *next() override { return this; }
  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) override {
    auto pc = quack::adjust_aspect(
        {
            .grid_pos = {0.0f, 0.5f},
            .grid_size = {1.0f, 1.0f},
        },
        sw->aspect());

    auto rp = sw->cmd_render_pass({
        .command_buffer = *pcb,
        .clear_color = {{0, 0, 0, 1}},
    });
    m_ib.build_commands(*pcb);
    m_ps.cmd_push_vert_frag_constants(*pcb, pc);

    m_ps.cmd_bind_descriptor_set(*pcb, m_bg.dset());
    m_ps.run(*pcb, 1, 0);

    m_ps.cmd_bind_descriptor_set(*pcb, m_logo.dset());
    m_ps.run(*pcb, 1, 1);

    m_ps.cmd_bind_descriptor_set(*pcb, m_sel.dset());
    m_ps.run(*pcb, 9, 7);

    m_ps.cmd_bind_descriptor_set(*pcb, m_texts.dset());
    m_ps.run(*pcb, 5, 2);
  }

  void key_down(casein::keys k) override {
    if (alpha() < 1.0f)
      return;

    if (k == casein::K_DOWN)
      m_idx = (m_idx + 1) % 5;
    if (k == casein::K_UP)
      m_idx = (m_idx + 4) % 5;
  }
};

struct splash_2 : splash {
  splash_2(voo::device_and_queue *dq) : splash{dq, "Lenna_(test_image).png"} {}

  scene *create_next() override { return new main_menu{device_and_queue()}; }
};
struct splash_1 : splash {
  splash_1(voo::device_and_queue *dq) : splash{dq, "BrainF.png"} {}

  scene *create_next() override { return new splash_2{device_and_queue()}; }
};

class renderer : public voo::casein_thread {
  hai::uptr<scene> *m_s;

public:
  void run() override {
    voo::device_and_queue dq{"hide", native_ptr()};

    hai::uptr<scene> s{new main_menu{&dq}};
    m_s = &s;
    release_init_lock();

    sith::memfn_thread<scene> thr{&*s, &scene::run};
    thr.start();

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      extent_loop(dq, sw, [&] {
        sw.queue_one_time_submit(dq, [&](auto pcb) { s->run(&sw, pcb); });

        auto n = s->next();
        if (n != &*s) {
          thr = {n, &scene::run};
          s.reset(n);
          thr.start();
        }
      });
    }
  }

  void key_down(const casein::events::key_down &e) override {
    wait_init();
    (*m_s)->key_down(*e);
  }

  static auto &instance() {
    static renderer r{};
    return r;
  }
};

extern "C" void casein_handle(const casein::event &e) {
  renderer::instance().handle(e);
  quack::mouse_tracker::instance().handle(e);
}
