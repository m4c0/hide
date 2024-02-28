#pragma leco add_resource "VictorMono-Regular.otf"

export module hide:text;
import dotz;
import jute;
import vee;
import voo;
import what_the_font;

namespace hide {
static constexpr const auto font_h = 100;
static constexpr const auto line_h = 128;
auto &font() {
  static wtf::library wtf{};
  static wtf::face face{wtf.new_face("VictorMono-Regular.otf", font_h)};
  return face;
}
export class text {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::h2l_image m_img;
  vee::descriptor_set m_dset;
  int m_pen_y = font_h;

public:
  text(vee::physical_device pd, voo::queue *q, vee::descriptor_set dset)
      : m_img{pd, 1024, 1024, false}
      , m_dset{dset} {
    vee::update_descriptor_set(dset, 0, m_img.iv(), *m_smp);
  }

  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }

  void draw(jute::view str) {
    int pen_x = 0;

    voo::mapmem mem{m_img.host_memory()};
    auto img = static_cast<unsigned char *>(*mem);
    font().shape_en(str).draw(img, m_img.width(), m_img.height(), pen_x,
                              m_pen_y);

    m_pen_y += line_h;
  }

  void setup_copy(vee::command_buffer cb) { m_img.setup_copy(cb); }
};
} // namespace hide
