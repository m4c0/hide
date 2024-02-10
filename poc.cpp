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
static wtf::face g_wtf_face_100{g_wtf.new_face("VictorMono-Regular.otf", 100)};

void reset_quack(auto all, unsigned size) {
  for (auto i = 0; i < size; i++) {
    all.multipliers[i] = {1, 1, 1, 1};
    all.colours[i] = {0, 0, 0, 0};
  }
}
auto cmd_render_pass(voo::swapchain_and_stuff *sw, vee::command_buffer cb) {
  return sw->cmd_render_pass({
      .command_buffer = cb,
      .clear_color = {{0, 0, 0, 1}},
  });
}
auto cmd_push_vert_frag_constants(voo::swapchain_and_stuff *sw,
                                  vee::command_buffer cb,
                                  quack::pipeline_stuff *ps) {
  auto pc = quack::adjust_aspect(
      {
          .grid_pos = {0.0f, 0.5f},
          .grid_size = {1.0f, 1.0f},
      },
      sw->aspect());
  ps->cmd_push_vert_frag_constants(cb, pc);
}

class scene : public voo::update_thread {
  voo::device_and_queue *m_dq;

protected:
  [[nodiscard]] constexpr auto *device_and_queue() const noexcept {
    return m_dq;
  }

public:
  scene(voo::device_and_queue *dq) : update_thread{dq}, m_dq{dq} {}

  using update_thread::run;

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

class texts_shaper {
  static constexpr const auto line_h = 128;
  static constexpr const auto font_h = 100;

  voo::mapmem m_mem;
  unsigned m_w;
  unsigned m_h;
  int m_pen_y = font_h;
  float m_v0 = 0;
  float m_y = 0;
  quack::rect *m_pos;
  quack::uv *m_uvs;

public:
  texts_shaper(voo::h2l_image *img, quack::rect *pos, quack::uv *uvs)
      : m_mem{img->host_memory()}
      , m_w{img->width()}
      , m_h{img->height()}
      , m_pos{pos}
      , m_uvs{uvs} {}

  void draw(jute::view str, float h) {
    auto &f = g_wtf_face_100;

    auto img = static_cast<unsigned char *>(*m_mem);
    int pen_x = 0;
    f.shape_en(str).draw(img, m_w, m_h, &pen_x, &m_pen_y);

    float u1 = static_cast<float>(pen_x) / 1024.0f;
    float v1 = m_v0 + 0.125f;
    *m_uvs++ = {{0, m_v0}, {u1, v1}};

    float w = h * pen_x / line_h;
    *m_pos++ = {{0, m_y}, {w, h}};

    m_pen_y += line_h;
    m_v0 = v1;
    m_y += h;
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
  }

  [[nodiscard]] auto shaper(quack::rect *pos, quack::uv *uvs) noexcept {
    return texts_shaper{&m_img, pos, uvs};
  }
  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }

  using update_thread::run_once;
};

class splash : public scene {
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
  virtual scene *create_next() = 0;

public:
  splash(voo::device_and_queue *dq, jute::view name)
      : scene{dq}
      , m_ps{*dq, 1}
      , m_ib{m_ps.create_batch(1)}
      , m_img{dq, &m_ps, name} {
    m_ib.map_all([this](auto all) {
      reset_quack(all, 1);

      auto img_aspect = m_img.aspect();
      all.positions[0] = {{-img_aspect / 2.f, 0}, {img_aspect, 1}};
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
    auto rp = cmd_render_pass(sw, *pcb);
    m_ib.build_commands(*pcb);
    cmd_push_vert_frag_constants(sw, *pcb, &m_ps);
    m_ps.cmd_bind_descriptor_set(*pcb, m_img.dset());
    m_ps.run(*pcb, 1);
  }
};

class game : public scene {
  sitime::stopwatch m_time{};

  void build_cmd_buf(vee::command_buffer cb) override {}

public:
  using scene::scene;

  [[nodiscard]] scene *next() override;

  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) override {
    auto rp = cmd_render_pass(sw, *pcb);
  }
};

class background {
  image m_bg;

public:
  background(voo::device_and_queue *dq, quack::pipeline_stuff *ps)
      : m_bg{dq, ps, "m3-bg.png"} {}

  void set_all(auto all) {
    all.positions[0] = {{-2.f, -2.f}, {4.f, 4.f}};
    all.uvs[0] = {{0, 0}, {1, 1}};
  }

  void run(quack::pipeline_stuff *ps, vee::command_buffer cb) {
    ps->cmd_bind_descriptor_set(cb, m_bg.dset());
    ps->run(cb, 1, 0);
  }
};
class selection_bg {
  image m_sel;

  static constexpr const auto sel_border = 0.01f;

public:
  selection_bg(voo::device_and_queue *dq, quack::pipeline_stuff *ps)
      : m_sel{dq, ps, "m3-storeCounter_bar.png"} {}

  void set_pos(quack::rect *ps, const quack::rect &p) const {
    ps[0] = p;

    ps[1] = ps[3] = ps[4] = ps[6] = {{}, {sel_border, sel_border}};
    ps[2] = ps[5] = {{}, {sel_border, p.h}};
    ps[7] = ps[8] = {{}, {p.w, sel_border}};

    ps[1].x = ps[2].x = ps[3].x = p.x - sel_border;
    ps[7].x = ps[8].x = p.x;
    ps[4].x = ps[5].x = ps[6].x = p.x + p.w;

    ps[1].y = ps[4].y = ps[7].y = p.y - sel_border;
    ps[2].y = ps[5].y = p.y;
    ps[3].y = ps[6].y = ps[8].y = p.y + p.h;
  }
  void set_uvs(quack::uv *uvs) const {
    constexpr const float t = 0.05;
    constexpr const float b = 0.95;
    constexpr const float l = 0.05;
    constexpr const float r = 0.95;
    uvs[0] = {{l, t}, {r, b}};
    uvs[1] = {{0, 0}, {l, t}};
    uvs[2] = {{0, t}, {l, b}};
    uvs[3] = {{0, b}, {l, 1}};
    uvs[4] = {{r, 0}, {1, t}};
    uvs[5] = {{r, t}, {1, b}};
    uvs[6] = {{r, b}, {1, 1}};
    uvs[7] = {{l, 0}, {r, t}};
    uvs[8] = {{l, b}, {r, 1}};
  }

  void run(quack::pipeline_stuff *ps, vee::command_buffer cb, unsigned first) {
    ps->cmd_bind_descriptor_set(cb, m_sel.dset());
    ps->run(cb, 9, first);
  }
};

class options : public scene {
  enum items { o_sound, o_music, o_fullscreen, o_back, o_count };

  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  background m_bg;
  selection_bg m_sel;
  texts m_txt;

  static constexpr const auto max_dset = 3;
  static constexpr const auto max_sprites = 1 + o_count + 9;

  void build_cmd_buf(vee::command_buffer cb) override {
    voo::cmd_buf_one_time_submit pcb{cb};
    m_ib.setup_copy(cb);
  }

public:
  options(voo::device_and_queue *dq)
      : scene{dq}
      , m_ps{*dq, max_dset}
      , m_ib{m_ps.create_batch(max_sprites)}
      , m_bg{dq, &m_ps}
      , m_sel{dq, &m_ps}
      , m_txt{dq, &m_ps} {
    m_ib.map_all([this](auto all) {
      reset_quack(all, max_sprites);
      m_bg.set_all(all);

      auto s = m_txt.shaper(all.positions + 1, all.uvs + 1);
      s.draw("Sound", 0.0625f);
      s.draw("Music", 0.0625f);
      s.draw("Fullscreen", 0.0625f);
      s.draw("Back", 0.0625f);

      for (auto i = 0; i < o_count; i++) {
        all.positions[1 + i].x = -0.3f;
      }
      all.positions[1 + o_fullscreen].y += 0.02f;
      all.positions[1 + o_back].y += 0.02f * 2.f;

      m_sel.set_pos(all.positions + 1 + o_count, all.positions[1]);
      m_sel.set_uvs(all.uvs + 1 + o_count);
    });

    m_txt.run_once();
  }

  [[nodiscard]] scene *next() override { return this; }

  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) override {
    auto rp = cmd_render_pass(sw, *pcb);
    m_ib.build_commands(*pcb);
    cmd_push_vert_frag_constants(sw, *pcb, &m_ps);

    m_bg.run(&m_ps, *pcb);
    m_sel.run(&m_ps, *pcb, 1 + o_count);

    m_ps.cmd_bind_descriptor_set(*pcb, m_txt.dset());
    m_ps.run(*pcb, o_count, 1);
  }
};

// TODO: fix weird submitting empty CB
class main_menu : public scene {
  enum menu_options {
    o_new_game,
    o_continue,
    o_options,
    o_credits,
    o_exit,
    o_count
  };

  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  background m_bg;
  image m_logo;
  selection_bg m_sel;
  texts m_texts;

  sitime::stopwatch m_time{};
  bool m_selected{};

  bool m_has_save{};
  menu_options m_idx{};

  static constexpr const auto max_dset = 4;
  static constexpr const auto max_sprites = 2 + 5 + 9;
  static constexpr const auto menu_w = 0.25f;
  static constexpr const auto menu_h = 0.0625f;

  void build_cmd_buf(vee::command_buffer cb) override {
    auto a = alpha();
    m_ib.map_multipliers([this, a](auto *ms) {
      // TODO: show bg faster
      // TODO: "set alpha" in BG
      for (auto i = 0; i < max_sprites; i++)
        ms[i] = {1, 1, 1, a};

      for (auto i = 2; i < 7; i++) {
        float n = (m_idx == i - 2) ? 0 : 1;
        ms[i] = {n, 0, 0, a};
      }

      if (!m_has_save) {
        ms[3] = {0.6, 0.5, 0.5, a};
      }
      // TODO: set alpha in BG
      if (m_idx == o_options) {
        ms[0] = {1, 1, 1, 1};
      }
    });

    m_ib.map_positions([this](auto *ps) { setup_positions(ps); });

    voo::cmd_buf_one_time_submit pcb{cb};
    m_ib.setup_copy(cb);
  }

  [[nodiscard]] auto time() const noexcept { return m_time.millis() / 1000.0; }
  [[nodiscard]] float alpha() const noexcept {
    float t = time();
    t = t >= 1 ? 1.0 : t;
    return m_selected ? 1.0f - t : t;
  }

  void setup_positions(quack::rect *ps) const {
    float a = alpha();
    // TODO: easy in/out
    // TODO: other kind of animations
    // TODO: adjust for vertical screens
    float hs = 0.5f * (1.0f - a);

    // TODO: move "initial positions" out
    ps[1] = {{0, 0}, m_logo.size(0.5f)};
    for (auto i = 0; i < 5; i++) {
      ps[2 + i].y = ps[1 + i].y + ps[1 + i].h;
    }

    ps[1].y -= hs;
    for (auto i = 0; i < 5; i++) {
      ps[2 + i].y += hs + 0.05f;
    }

    auto h = 1.f - 0.05f;
    for (auto i = 1; i < 7; i++) {
      ps[i].x = -ps[i].w / 2.0f;
      h -= ps[i].h;
    }
    for (auto i = 1; i < 7; i++) {
      ps[i].y += h / 2.0f;
    }

    m_sel.set_pos(ps + 7, ps[m_idx + 2]);
  }

  using update_thread::run;

public:
  explicit main_menu(voo::device_and_queue *dq, bool has_save = false)
      : scene{dq}
      , m_ps{*dq, max_dset}
      , m_ib{m_ps.create_batch(max_sprites)}
      , m_bg{dq, &m_ps}
      , m_logo{dq, &m_ps, "m3-game_title.png"}
      , m_sel{dq, &m_ps}
      , m_texts{dq, &m_ps}
      , m_has_save{has_save} {
    m_ib.map_all([this](auto all) {
      reset_quack(all, max_sprites);
      m_bg.set_all(all);

      all.uvs[1] = {{0, 0}, {1, 1}};

      auto s = m_texts.shaper(all.positions + 2, all.uvs + 2);
      s.draw("New Game", menu_h);
      s.draw("Continue", menu_h);
      s.draw("Options", menu_h);
      s.draw("Credits", menu_h);
      s.draw("Exit", menu_h);

      m_sel.set_uvs(all.uvs + 7);
    });

    m_ib.map_positions([this](auto *ps) { setup_positions(ps); });

    m_texts.run_once();
  }

  scene *next() override {
    if (!m_selected || time() < 1)
      return this;

    switch (m_idx) {
    case o_new_game:
    case o_continue:
      return new game{device_and_queue()};
    case o_options:
      return new options{device_and_queue()};
    default:
      return new main_menu{device_and_queue(), !m_has_save};
    }
  }
  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) override {
    auto rp = cmd_render_pass(sw, *pcb);
    m_ib.build_commands(*pcb);
    cmd_push_vert_frag_constants(sw, *pcb, &m_ps);

    m_bg.run(&m_ps, *pcb);

    m_ps.cmd_bind_descriptor_set(*pcb, m_logo.dset());
    m_ps.run(*pcb, 1, 1);

    m_sel.run(&m_ps, *pcb, 7);

    m_ps.cmd_bind_descriptor_set(*pcb, m_texts.dset());
    m_ps.run(*pcb, o_count, 2);
  }

  void key_down(casein::keys k) override {
    if (alpha() < 1.0f)
      return;

    do {
      if (k == casein::K_DOWN)
        m_idx = static_cast<menu_options>((m_idx + 1) % 5);
      if (k == casein::K_UP)
        m_idx = static_cast<menu_options>((m_idx + 4) % 5);
      if (k == casein::K_ENTER) {
        m_time = {};
        m_selected = true;
      }
    } while (!m_has_save && m_idx == o_continue);
  }
};
scene *game::next() {
  if (m_time.millis() > 2000)
    return new main_menu{device_and_queue()};
  return this;
}

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

    hai::uptr<scene> s{new options{&dq}};
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
