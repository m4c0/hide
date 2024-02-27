export module hide:image;
import jute;
import quack;
import vee;
import voo;

namespace hide {
export class image {
  vee::sampler m_smp = vee::create_sampler(vee::linear_sampler);
  voo::sires_image m_img;
  vee::descriptor_set m_dset;

public:
  image(vee::physical_device pd, voo::queue *q, vee::descriptor_set dset,
        jute::view name)
      : m_img{name, pd, q}
      , m_dset{dset} {
    vee::update_descriptor_set(dset, 0, m_img.iv(), *m_smp);
    m_img.run_once();
  }
  image(vee::physical_device pd, voo::queue *q, quack::pipeline_stuff *ps,
        jute::view name)
      : m_img{name, pd, q}
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
