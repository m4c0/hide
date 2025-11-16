#pragma leco app
#pragma leco add_shader "poc-2.vert"
#pragma leco add_shader "poc-2.frag"

import casein;
import dotz;
import hai;
import vinyl;
import voo;

struct inst {
  dotz::vec2 pos;
  dotz::vec4 colour;
};
static constexpr const auto max_inst = 1024;

struct as {
  voo::device_and_queue dq { "poc2-hide", casein::native_ptr };
  vee::render_pass rp = voo::single_att_render_pass(dq);

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  voo::bound_buffer buf = voo::bound_buffer::create_from_host(
      dq.physical_device(), max_inst * sizeof(inst),
      vee::buffer_usage::vertex_buffer);

  voo::one_quad quad { dq.physical_device() };
};
hai::uptr<as> gas {};

struct ss {
  voo::swapchain_and_stuff sw { gas->dq, *gas->rp };

  vee::gr_pipeline gp = vee::create_graphics_pipeline({
    .pipeline_layout = *gas->pl,
    .render_pass = *gas->rp,
    .extent = sw.extent(),
    .shaders {
      voo::shader { "poc-2.vert.spv" }.pipeline_vert_stage("main", vee::specialisation_info<float>(0, sw.aspect())),
      voo::shader { "poc-2.frag.spv" }.pipeline_frag_stage("main"),
    },
    .bindings {
      vee::vertex_input_bind(sizeof(dotz::vec2)),
      vee::vertex_input_bind_per_instance(sizeof(inst)),
    },
    .attributes {
      vee::vertex_attribute_vec2(0, 0),
      vee::vertex_attribute_vec2(1, traits::offset_of(&inst::pos)),
      vee::vertex_attribute_vec4(1, traits::offset_of(&inst::colour)),
    },
  });
};
hai::uptr<ss> gss {};

static void run(voo::memiter<inst> & m);

static unsigned map() {
  unsigned count = 0;
  voo::memiter<inst> m { *gas->buf.memory, &count };
  run(m);
  return count;
}

static void on_frame() {
  if (!gss) gss.reset(new ss {});

  auto count = map();

  gss->sw.acquire_next_image();
  gss->sw.queue_one_time_submit(gas->dq.queue(), [&] {
    auto cb = gss->sw.command_buffer();
    auto rp = gss->sw.cmd_render_pass({
      .clear_colours { vee::clear_colour(0.01f, 0.02f, 0.03f, 1.0f) },
    });
    vee::cmd_bind_gr_pipeline(cb, *gss->gp);
    vee::cmd_bind_vertex_buffers(cb, 1, *gas->buf.buffer);
    gas->quad.run(cb, 0, count);
  });
  gss->sw.queue_present(gas->dq.queue());
}

const int i = [] {
  using namespace vinyl;
  on(START, [] { gas.reset(new as {}); });
  on(STOP, [] { gss = {}; gas = {}; });
  on(FRAME, on_frame);
  on(RESIZE, [] { gss = {}; });
  return 0;
}();

struct widget {
  inst inst {};
  dotz::vec2 size {};

  void (*layout)(widget *);

  hai::varray<widget *> children { 128 };
};
static void layout(widget * node) {
  for (auto c : node->children) layout(c);

  if (node->layout) node->layout(node);
}
static void tr(widget * node, dotz::vec2 delta) {
  for (auto c : node->children) tr(c, delta);
  node->inst.pos = node->inst.pos + delta;
}
namespace l {
  static void hbox(widget * node) {
    float x = 0;
    for (auto c : node->children) {
      tr(c, { x, 0.f });
      x += c->size.x;
      node->size.y = dotz::max(node->size.y, c->size.y);
    }
    node->size.x = x;
  }
  static void vbox(widget * node) {
    float y = 0;
    for (auto c : node->children) {
      tr(c, { 0.f, y });
      y += c->size.y;
      node->size.x = dotz::max(node->size.x, c->size.x);
    }
    node->size.y = y;
  }
}
static void render(voo::memiter<inst> & m, widget * node) {
  if (node->children.size() == 0) m += node->inst;

  for (auto c : node->children) render(m, c);
}

static void run(voo::memiter<inst> & m) {
  static hai::varray<widget> arena { 128 };
  arena.truncate(0);

  const auto alloc = [] {
    arena.push_back(widget {});
    return &arena[arena.size() - 1];
  };

  auto vbox = alloc();
  vbox->layout = l::vbox;

  auto top_nav = alloc();
  top_nav->layout = l::hbox;

  auto lt_btn = alloc();
  lt_btn->inst.colour = { 1, 0, 0, 1 };
  lt_btn->size = { 1 };
  top_nav->children.push_back(lt_btn);

  auto title = alloc();
  title->inst.colour = { 0, 1, 0, 1 };
  title->size = { 1 };
  top_nav->children.push_back(title);

  auto rt_btn = alloc();
  rt_btn->inst.colour = { 0, 0, 1, 1 };
  rt_btn->size = { 1 };
  top_nav->children.push_back(rt_btn);

  vbox->children.push_back(top_nav);

  auto cnt = alloc();
  cnt->inst.colour = { 0.5f, 0.5f, 0.5f, 1.0f };
  cnt->size = { 1 };
  vbox->children.push_back(cnt);

  auto bot_nav = alloc();
  bot_nav->layout = l::hbox;

  auto tab1 = alloc();
  tab1->inst.colour = { 1, 1, 0, 1 };
  tab1->size = { 1 };
  bot_nav->children.push_back(tab1);

  auto tab2 = alloc();
  tab2->inst.colour = { 1, 0, 1, 1 };
  tab2->size = { 1 };
  bot_nav->children.push_back(tab2);

  auto tab3 = alloc();
  tab3->inst.colour = { 0, 1, 1, 1 };
  tab3->size = { 1 };
  bot_nav->children.push_back(tab3);

  auto tab4 = alloc();
  tab4->inst.colour = { 0, 0, 0, 1 };
  tab4->size = { 1 };
  bot_nav->children.push_back(tab4);

  auto tab5 = alloc();
  tab5->inst.colour = { 1, 1, 1, 1 };
  tab5->size = { 1 };
  bot_nav->children.push_back(tab5);

  vbox->children.push_back(bot_nav);

  layout(vbox);
  render(m, vbox);
}
