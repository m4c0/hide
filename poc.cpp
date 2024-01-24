#pragma leco app
export module poc;

import casein;
import silog;
import quack;
import vee;
import voo;

void atlas_image(quack::u8_rgba *img) {
  for (auto i = 0; i < 16 * 16; i++) {
    auto x = (i / 16) % 2;
    auto y = (i % 16) % 2;
    unsigned char b = (x ^ y) == 0 ? 255 : 0;

    img[i] = {255, 255, 255, 0};
    img[i + 256] = {b, b, b, 128};
  }
}

constexpr const auto max_batches = 100;
class renderer : public voo::casein_thread {
  quack::instance_batch *m_ib;

public:
  void run() override {
    voo::device_and_queue dq{"quack", native_ptr()};

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      quack::pipeline_stuff ps{dq, sw, max_batches};
      auto ib = ps.create_batch(2);

      ib.load_atlas(16, 32, atlas_image);
      ib.map_positions([](auto *ps) {});
      ib.map_colours([](auto *cs) {});
      ib.map_uvs([](auto *us) {});
      ib.map_multipliers([](auto *ms) {});
      ib.center_at(0.5, 0.5);
      ib.set_count(1);
      ib.set_grid(1, 1);

      m_ib = &ib;
      release_init_lock();
      extent_loop(dq, sw, [&] {
        ib.submit_buffers(dq.queue());

        sw.one_time_submit(dq, [&](auto &pcb) {
          auto scb = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_color = {{0, 0, 0, 1}},
          });
          ps.run(*scb, ib);
        });
      });
    }
  }

  static auto &instance() {
    static renderer r{};
    return r;
  }
};

class main_loop : public voo::casein_thread {
public:
  void run() override {
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
  }

  static auto &instance() {
    static main_loop r{};
    return r;
  }
};

extern "C" void casein_handle(const casein::event &e) {
  main_loop::instance().handle(e);
  renderer::instance().handle(e);
  quack::mouse_tracker::instance().handle(e);
}
