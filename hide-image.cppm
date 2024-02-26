export module hide:image;
import jute;
import quack;
import vee;
import voo;

namespace hide {
// TODO: internalise image or remove quack dependency
export class image {
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

} // namespace hide
