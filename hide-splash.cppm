export module hide:splash;
import :image;
import jute;
import quack;
import sith;
import sitime;
import vee;
import voo;

namespace hide {
export class splash {
  quack::pipeline_stuff m_ps;
  quack::buffer_updater m_ib;
  image m_img;

  sitime::stopwatch m_time{};

  sith::run_guard m_run{};

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

  void load(quack::instance *& i) {
    auto img_aspect = m_img.aspect();

    *i++ = quack::instance {
      .position { -img_aspect / 2.f, 0.f },
      .size { img_aspect, 1.f },
      .uv0 { 0 },
      .uv1 { 1 },
      .multiplier { 1.f, 1.f, 1.f, alpha() },
    };
  }

public:
  splash(voo::device_and_queue *dq, jute::view name) :
    m_ps { *dq, 1 }
  , m_ib { dq, 1, [this](auto & i) { this->load(i); } }
  , m_img { dq->physical_device(), dq->queue(), m_ps.allocate_descriptor_set(), name }
  , m_run { &m_ib }
  {}

  [[nodiscard]] bool done() { return time() > 3; }

  void render(voo::swapchain_and_stuff * sw) {
    quack::upc pc {
      .grid_pos { 0.0f, 0.5f },
      .grid_size { 1.0f, 1.0f },
    };
    quack::run(&m_ps, {
      .sw = sw,
      .pc = &pc,
      .inst_buffer = m_ib.data().local_buffer(),
      .atlas_dset = m_img.dset(),
      .count = 1,
    });
  }
};
} // namespace hide
