export module hide:image;
import dotz;
import jute;
import vee;
import voo;

namespace hide {
export class image {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::bound_image m_img;
  vee::descriptor_set m_dset;
  float m_aspect;

public:
  image(vee::physical_device pd, voo::queue * q, vee::descriptor_set dset, jute::view name) :
    m_dset { dset }
  {
    voo::load_image(name, pd, q, &m_img, [=,this](auto size) {
      vee::update_descriptor_set(dset, 0, *m_img.iv, *m_smp);
      m_aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
    });
  }

  [[nodiscard]] constexpr auto dset() const noexcept { return m_dset; }
  [[nodiscard]] constexpr auto aspect() const noexcept { return m_aspect; }

  [[nodiscard]] constexpr auto size(float h) const noexcept {
    return dotz::vec2{h * aspect(), h};
  }
};
} // namespace hide
