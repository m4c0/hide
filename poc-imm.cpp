#pragma leco app

import casein;
import dotz;
import sith;
import sitime;
import vee;
import voo;

namespace splash {
[[nodiscard]] float alpha(const sitime::stopwatch &time) {
  auto t = time.millis();
  if (t < 1.0)
    return t;
  if (t < 2.0)
    return 1.0f;
  if (t < 3.0)
    return 3.0f - t;
  return 0.0f;
}
} // namespace splash

struct rect {
  dotz::vec2 pos;
  dotz::vec2 size;
  float alpha;
};

class layout_thread : public voo::update_thread {
  static constexpr const auto max_quads = 10240;

  voo::h2l_buffer m_buffer;
  sitime::stopwatch m_time{};

  void build_cmd_buf(vee::command_buffer cb) override {
    voo::mapmem m{m_buffer.host_memory()};
    auto buf = static_cast<rect *>(*m);

    float alpha = splash::alpha(m_time);
    *buf++ = {{0, 0}, {1, 1}, alpha};

    voo::cmd_buf_one_time_submit pcb{cb};
    m_buffer.setup_copy(cb);
  }

public:
  explicit layout_thread(vee::physical_device pd, voo::queue *q)
      : update_thread{q}
      , m_buffer{pd, max_quads * sizeof(rect)} {
    run_once();
  }
};

class thread : public voo::casein_thread {
  void run() override {
    voo::device_and_queue dq{"hide-immediate", native_ptr()};

    voo::one_quad quad{dq.physical_device()};
    layout_thread layout{dq.physical_device(), dq.queue()};

    while (!interrupted()) {
      voo::swapchain_and_stuff sw{dq};

      sith::run_guard lr{&layout};

      extent_loop(dq.queue(), sw, [&] {
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto rp = sw.cmd_render_pass({
              .command_buffer = *pcb,
              .clear_color = {},
          });
        });
      });
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  t.handle(e);
}
