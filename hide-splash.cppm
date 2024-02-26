export module hide:splash;
import :image;
import jute;
import quack;
import sith;
import sitime;
import vee;
import voo;

namespace hide {
export class splash : public voo::update_thread {
  quack::pipeline_stuff m_ps;
  quack::instance_batch m_ib;
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

  void build_cmd_buf(vee::command_buffer cb) override {
    auto a = alpha();
    m_ib.map_multipliers([a](auto *ms) { *ms = {1, 1, 1, a}; });

    voo::cmd_buf_one_time_submit pcb{cb};
    m_ib.setup_copy(cb);
  }

  using update_thread::run;

public:
  splash(voo::device_and_queue *dq, jute::view name)
      : splash(dq->queue(), dq->physical_device(), dq->render_pass(), name) {}
  splash(voo::queue *q, vee::physical_device pd, vee::render_pass::type rp,
         jute::view name)
      : update_thread{q}
      , m_ps{pd, rp, 1}
      , m_ib{m_ps.create_batch(1)}
      , m_img{pd, q, &m_ps, name} {
    m_ib.map_all([this](auto all) {
      auto img_aspect = m_img.aspect();
      all.positions[0] = {{-img_aspect / 2.f, 0}, {img_aspect, 1}};
      all.multipliers[0] = {1, 1, 1, 1};
      all.colours[0] = {0, 0, 0, 0};
      all.uvs[0] = {{0, 0}, {1, 1}};
    });
    m_run = sith::run_guard{this};
  }

  [[nodiscard]] bool done() { return time() > 3; }

  void render(vee::command_buffer cb, float aspect) {
    auto pc = quack::adjust_aspect(
        {
            .grid_pos = {0.0f, 0.5f},
            .grid_size = {1.0f, 1.0f},
        },
        aspect);
    m_ib.build_commands(cb);
    m_ps.cmd_push_vert_frag_constants(cb, pc);
    m_ps.cmd_bind_descriptor_set(cb, m_img.dset());
    m_ps.run(cb, 1);
  }
};
} // namespace hide
