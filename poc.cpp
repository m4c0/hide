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

class splash {
  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
  jute::view m_file;
  sitime::stopwatch m_time{};
  sith::memfn_thread<splash> m_thread{this, &splash::run};

  void run(sith::thread *) {
    m_ib.load_atlas(m_file);
    m_ib.set_count(1);

    float t{};
    while (t < 3 && !m_thread.interrupted()) {
      t = m_time.millis() / 1000.0;
      m_ib.map_multipliers([t](auto *ms) {
        float a = 1.0;
        if (t < 1) {
          a = t;
        } else if (t > 2) {
          a = 3.0 - t;
          if (a < 0)
            a = 0;
        }
        ms[0] = {1, 1, 1, a};
      });
    }
  }

public:
  splash(quack::pipeline_stuff &&ps, jute::view res)
      : m_ps{traits::move(ps)}, m_ib{m_ps.create_batch(1)}, m_file{res} {
    m_ib.map_all([](auto all) {
      all.positions[0] = {{0, 0}, {1, 1}};
      all.multipliers[0] = {};
      all.colours[0] = {};
      all.uvs[0] = {{0, 0}, {1, 1}};
    });
    m_ib.center_at(0.5, 0.5);
    m_ib.set_grid(1, 1);
    m_ib.set_count(0);
    m_thread.start();
  }
  ~splash() { vee::device_wait_idle(); }

  [[nodiscard]] splash *next() noexcept {
    auto t = m_time.millis() / 1000.0;
    if (t > 3) {
      return new splash(traits::move(m_ps), "Lenna_(test_image).png");
    }
    return this;
  }

  void submit_buffers(vee::queue q) { m_ib.submit_buffers(q); }
  void run(voo::swapchain_and_stuff &sw, vee::command_buffer cb) {
    auto scb = sw.cmd_render_pass({
        .command_buffer = cb,
        .clear_color = {{0, 0, 0, 1}},
    });
    m_ps.run(*scb, m_ib);
  }
};

constexpr const auto max_batches = 100;
class renderer : public voo::casein_thread {
public:
  void run() override {
    voo::device_and_queue dq{"quack", native_ptr()};

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      quack::pipeline_stuff ps{dq, sw, max_batches};

      auto s = hai::uptr<splash>::make(traits::move(ps), "BrainF.png");

      release_init_lock();
      extent_loop(dq, sw, [&] {
        s->submit_buffers(dq.queue());

        sw.one_time_submit(dq, [&](auto &pcb) { s->run(sw, *pcb); });

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
