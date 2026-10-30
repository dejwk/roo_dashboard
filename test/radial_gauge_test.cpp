#include "roo_dashboard/meters/radial_gauge.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "roo_display/core/offscreen.h"
#include "roo_display/ui/text_label.h"
#include "roo_fonts/NotoSans_Condensed/15.h"
#include "roo_windows/containers/static_layout.h"
#include "roo_windows/core/application.h"

namespace {
using roo_display::Color;
class CountingDisplay
    : public roo_display::OffscreenDevice<roo_display::Argb4444> {
 public:
  explicit CountingDisplay(roo::byte* data, int16_t width = 160,
                           int16_t height = 128)
      : OffscreenDevice(width, height, data, roo_display::Argb4444()),
        writes(width * height),
        stride_(width) {}
  void reset() { std::fill(writes.begin(), writes.end(), 0); }
  void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  roo_display::BlendingMode mode) override {
    left = x = x0;
    right = x1;
    y = y0;
    OffscreenDevice::setAddress(x0, y0, x1, y1, mode);
  }
  void count(uint32_t n) {
    while (n-- > 0) {
      ++writes[y * stride_ + x];
      if (++x > right) {
        x = left;
        ++y;
      }
    }
  }
  void write(Color* colors, uint32_t n) override {
    count(n);
    OffscreenDevice::write(colors, n);
  }
  void fill(Color color, uint32_t n) override {
    count(n);
    OffscreenDevice::fill(color, n);
  }
  void writePixels(roo_display::BlendingMode mode, Color* colors, int16_t* xs,
                   int16_t* ys, uint16_t n) override {
    for (int i = 0; i < n; ++i) {
      setAddress(xs[i], ys[i], xs[i], ys[i], mode);
      write(colors + i, 1);
    }
  }
  void fillPixels(roo_display::BlendingMode mode, Color color, int16_t* xs,
                  int16_t* ys, uint16_t n) override {
    for (int i = 0; i < n; ++i) {
      setAddress(xs[i], ys[i], xs[i], ys[i], mode);
      fill(color, 1);
    }
  }
  void writeRects(roo_display::BlendingMode mode, Color* colors, int16_t* x0,
                  int16_t* y0, int16_t* x1, int16_t* y1, uint16_t n) override {
    for (int i = 0; i < n; ++i) {
      setAddress(x0[i], y0[i], x1[i], y1[i], mode);
      fill(colors[i], (x1[i] - x0[i] + 1) * (y1[i] - y0[i] + 1));
    }
  }
  void fillRects(roo_display::BlendingMode mode, Color color, int16_t* x0,
                 int16_t* y0, int16_t* x1, int16_t* y1, uint16_t n) override {
    for (int i = 0; i < n; ++i) {
      setAddress(x0[i], y0[i], x1[i], y1[i], mode);
      fill(color, (x1[i] - x0[i] + 1) * (y1[i] - y0[i] + 1));
    }
  }
  std::vector<uint8_t> writes;

 private:
  int32_t stride_;
  int left = 0;
  int right = 0;
  int x = 0;
  int y = 0;
};

class CoveredPage : public roo_windows::StaticLayout,
                    public roo_windows::Destination {
 public:
  using StaticLayout::StaticLayout;
  roo_windows::Widget& getContents() override { return *this; }
  Color background() const override { return roo_display::color::Blue; }
};

// Verifies initial, moving-needle, invalidated and navigation-return frames
// have identical pixels to a fresh draw and settle every pixel at most once.
TEST(RadialGaugeTest, ClearsBackgroundAndRestoresNeedleWithoutTrails) {
  using namespace roo_windows;
  for (Color background :
       {Color(roo_display::color::White), Color(0xFFEEEEEE)}) {
    std::vector<roo::byte> pixels(160 * 128 * 2);
    CountingDisplay device(pixels.data());
    roo_display::Display display(device);
    roo_scheduler::SchedulingService scheduler;
    Theme theme;
    theme.framework.canvas = background;
    theme.framework.surface = background;
    Environment environment(scheduler, theme);
    auto app = std::make_unique<Application>(&environment, display);
    app->refresh();
    roo_display::TextLabel face("bar",
                                roo_display::font_NotoSans_Condensed_15(),
                                roo_display::color::Black);
    roo_dashboard::RadialGauge gauge(app->context());
    gauge.setFace(&face);
    gauge.setBounds(roo_display::Box(0, 0, 159, 79));
    gauge.setCenter(80, 150);
    gauge.setScaleRadius(100);
    gauge.setRangeAngles(-30, 30);
    gauge.setValueRange(0, 5);
    gauge.setDividers(1, 5);
    gauge.setScaleWidth(15);
    Task& task = app->addTask(gauge, roo_display::Box(0, 20, 159, 99));
    CoveredPage cover(app->context());
    auto snapshot = [&] {
      std::vector<Color> colors(160 * 80);
      for (int16_t y = 20; y < 100; ++y)
        for (int16_t x = 0; x < 160; ++x)
          device.raster().readColors(&x, &y, 1, &colors[(y - 20) * 160 + x]);
      return colors;
    };
    auto render = [&] {
      device.reset();
      app->refresh();
      EXPECT_LE(*std::max_element(device.writes.begin(), device.writes.end()),
                1);
    };
    // An invalidation must replace all previously displayed content.
    device.fillRect(roo_display::BlendingMode::kSource, 0, 20, 159, 99,
                    roo_display::color::Magenta);
    render();
    std::vector<Color> initial = snapshot();
    EXPECT_EQ(initial.front(), background);
    EXPECT_EQ(std::count(initial.begin(), initial.end(),
                         Color(roo_display::color::Magenta)),
              0);
    for (float value : {1.0f, 2.5f, 4.8f, 0.1f}) {
      SCOPED_TRACE(value);
      gauge.setValue(value);
      render();
      std::vector<Color> moving = snapshot();
      device.fillRect(roo_display::BlendingMode::kSource, 0, 20, 159, 99,
                      roo_display::color::Magenta);
      gauge.invalidateInterior();
      render();
      EXPECT_TRUE(moving == snapshot());
    }
    std::vector<Color> before_menu = snapshot();
    task.navigation().push(cover);
    render();
    task.navigation().pop();
    render();
    EXPECT_TRUE(before_menu == snapshot());
    task.navigation().push(cover);
    render();
    gauge.setValue(3.5f);
    task.navigation().pop();
    render();
    std::vector<Color> restored = snapshot();
    device.fillRect(roo_display::BlendingMode::kSource, 0, 20, 159, 99,
                    roo_display::color::Magenta);
    gauge.invalidateInterior();
    render();
    EXPECT_TRUE(restored == snapshot());
    EXPECT_NE(before_menu, restored);
    app.reset();
  }
}
}  // namespace
