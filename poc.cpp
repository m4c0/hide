#pragma leco app
#pragma leco add_resource "BrainF.png"
#pragma leco add_resource "Lenna_(test_image).png"
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

class splash : voo::update_thread {
  voo::sires_image m_img;
  vee::sampler m_smp = vee::create_sampler(vee::nearest_sampler);

  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  vee::descriptor_set m_dset;

  sitime::stopwatch m_time{};
  sith::memfn_thread<splash> m_thread{this, &splash::run};

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
  virtual splash *create_next() noexcept = 0;

public:
  splash(voo::device_and_queue *dq, jute::view name)
      : update_thread{dq}
      , m_img{name, dq}
      , m_ps{*dq, 1}
      , m_ib{m_ps.create_batch(1)}
      , m_dset{m_ps.allocate_descriptor_set(m_img.iv(), *m_smp)} {
    m_img.run_once();

    m_ib.map_all([this](auto all) {
      auto img_aspect = static_cast<float>(m_img.width()) /
                        static_cast<float>(m_img.height());
      all.positions[0] = {{-img_aspect / 2.f, 0}, {img_aspect, 1}};
      all.multipliers[0] = {1, 1, 1, 1};
      all.colours[0] = {0, 0, 0, 1};
      all.uvs[0] = {{0, 0}, {1, 1}};
    });

    m_thread.start();
  }
  virtual ~splash() = default;

  [[nodiscard]] splash *next() noexcept {
    if (time() > 3)
      return create_next();
    return this;
  }

  void run(voo::swapchain_and_stuff *sw,
           const voo::cmd_buf_one_time_submit &pcb) {
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
    m_ps.cmd_bind_descriptor_set(*pcb, m_dset);
    m_ps.run(*pcb, 1);
  }
};

struct globals {
  voo::device_and_queue *dq;
  voo::swapchain_and_stuff *sw;
} g_g;

struct splash_2 : splash {
  splash_2() : splash{g_g.dq, "Lenna_(test_image).png"} {}

  splash *create_next() noexcept { return new splash_2{}; }
};
struct splash_1 : splash {
  splash_1() : splash{g_g.dq, "BrainF.png"} {}

  splash *create_next() noexcept { return new splash_2{}; }
};

class renderer : public voo::casein_thread {
public:
  void run() override {
    voo::device_and_queue dq{"hide", native_ptr()};
    g_g.dq = &dq;

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};
      g_g.sw = &sw;
      release_init_lock();

      hai::uptr<splash> s{new splash_1{}};

      extent_loop(dq, sw, [&] {
        sw.queue_one_time_submit(dq, [&](auto pcb) { s->run(&sw, pcb); });

        s.reset(s->next());
      });
    }
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
