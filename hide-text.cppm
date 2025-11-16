#pragma leco add_resource "VictorMono-Regular.otf"

export module hide:text;
import dotz;
import hai;
import jute;
import traits;
import vee;
import voo;
import what_the_font;

namespace hide {
static constexpr const auto font_h = 100;
static constexpr const auto line_h = 128;
auto &font() {
  static wtf::library wtf{};
  static wtf::face face{wtf.new_face("VictorMono-Regular.otf")};
  return face;
}
export class text : voo::update_thread {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::h2l_image m_img;
  vee::descriptor_set m_dset;
  int m_pen_y = font_h;

  void build_cmd_buf(vee::command_buffer cb) override {
    voo::cmd_buf_one_time_submit pcb{cb};
    m_img.setup_copy(cb);
  }

public:
  text(vee::physical_device pd, voo::queue *q, vee::descriptor_set dset)
      : update_thread{q}
      , m_img { pd, 1024, 1024, VK_FORMAT_R8G8B8A8_SRGB }
      , m_dset{dset} {
    vee::update_descriptor_set(dset, 0, m_img.iv(), *m_smp);
  }

  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }

  [[nodiscard]] dotz::vec4 draw(jute::view str) {
    int pen_x = 0;

    voo::mapmem mem{m_img.host_memory()};
    auto img = static_cast<unsigned char *>(*mem);
    font().shape_en(str).draw(img, m_img.width(), m_img.height(), &pen_x,
                              &m_pen_y);

    float v0 = (m_pen_y - font_h) / 1024.0f;
    m_pen_y += line_h;
    return {0.0f, v0, pen_x / 1024.0f, line_h / 1024.0f};
  }

  using update_thread::run_once;
};
} // namespace hide
