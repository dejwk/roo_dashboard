#include "roo_dashboard/meters/modern_radial_gauge.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "roo_display/core/offscreen.h"
#include "roo_windows/containers/static_layout.h"
#include "roo_windows/core/application.h"

namespace {
using roo_dashboard::ModernRadialGauge;
using roo_display::Color;
using namespace roo_windows;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTurn = 2 * kPi;

class CountingDisplay
    : public roo_display::OffscreenDevice<roo_display::Argb4444> {
 public:
  explicit CountingDisplay(roo::byte* data, int16_t width = 160,
                           int16_t height = 128)
      : OffscreenDevice(width, height, data, roo_display::Argb4444()),
        writes(width * height),
        stride_(width) {}
  void reset() {
    std::fill(writes.begin(), writes.end(), 0);
    addresses = 0;
  }
  void setAddress(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  roo_display::BlendingMode mode) override {
    ++addresses;
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
  int addresses = 0;

 private:
  int32_t stride_;
  int left = 0;
  int right = 0;
  int x = 0;
  int y = 0;
};

class Backdrop : public StaticLayout, public Destination {
 public:
  Backdrop(ApplicationContext& context, bool pattern = false)
      : StaticLayout(context), pattern_(pattern) {}

  Widget& getContents() override { return *this; }
  Color background() const override { return roo_display::color::White; }

  void paint(PaintContext& ctx) const override {
    auto pattern = roo_display::MakeRasterizable(
        bounds().asBox(), [this](int16_t x, int16_t y) {
          return pattern_ && ((x / 8 + y / 8) % 2 != 0)
                     ? Color(0xFFDDDDDD)
                     : Color(roo_display::color::White);
        });
    ctx.drawObject(pattern);
  }

 private:
  bool pattern_;
};

class ValueColoredGauge : public ModernRadialGauge {
 public:
  using ModernRadialGauge::ModernRadialGauge;
  Color getScaleColor() const override {
    return value() < 0.5f ? roo_display::color::Blue
                          : roo_display::color::Yellow;
  }
  Color getValueColor() const override {
    return value() < 0.5f ? roo_display::color::Green : roo_display::color::Red;
  }
};

class ModernGaugeTest : public testing::Test {
 protected:
  ModernGaugeTest()
      : pixels_(160 * 128 * 2),
        device_(pixels_.data()),
        display_(device_),
        environment_(scheduler_),
        app_(std::make_unique<Application>(&environment_, display_)) {
    app_->refresh();
  }

  void TearDown() override { app_.reset(); }

  void install(std::unique_ptr<ModernRadialGauge> gauge, bool turbo = false,
               bool pattern = false) {
    if (turbo) display_.enableTurbo();
    gauge_ = gauge.get();
    auto page = std::make_unique<Backdrop>(app_->context(), pattern);
    page_ = page.get();
    page->add(std::move(gauge), Rect(10, 15, 109, 114));
    page_owner_ = std::move(page);
    task_ = &app_->addTask(*page_owner_, roo_display::Box(0, 0, 159, 127));
  }

  void render(bool force = false) {
    if (force) page_->invalidateInterior();
    device_.reset();
    app_->refresh();
    EXPECT_LE(*std::max_element(device_.writes.begin(), device_.writes.end()),
              1);
  }

  Color pixel(int16_t x, int16_t y) {
    Color result;
    device_.raster().readColors(&x, &y, 1, &result);
    return result;
  }

  std::vector<Color> snapshot() {
    std::vector<Color> result;
    for (int y = 0; y < 128; ++y)
      for (int x = 0; x < 160; ++x) result.push_back(pixel(x, y));
    return result;
  }

  int count(Color color) {
    roo_display::Argb4444 format;
    color = format.toArgbColor(format.fromArgbColor(color));
    std::vector<Color> colors = snapshot();
    return std::count(colors.begin(), colors.end(), color);
  }

  // Cardinal samples on the known full-circle centerline of a 100px square.
  Color atAngle(float angle) {
    float stroke =
        Scaled(std::max<float>(gauge_->scaleWidth(), gauge_->valueWidth()));
    float radius = (100 - stroke - 2) * 0.5f;
    return pixel(std::lround(59.5f + radius * std::cos(angle)),
                 std::lround(64.5f - radius * std::sin(angle)));
  }

  std::vector<roo::byte> pixels_;
  CountingDisplay device_;
  roo_display::Display display_;
  roo_scheduler::SchedulingService scheduler_;
  Environment environment_;
  std::unique_ptr<Application> app_;
  std::unique_ptr<Backdrop> page_owner_;
  ModernRadialGauge* gauge_ = nullptr;
  Backdrop* page_ = nullptr;
  Task* task_ = nullptr;
};

// Verifies normalized values, finite-input validation, default angles and dp
// widths, including fractional widths and zero-width/zero-sweep configurations.
TEST_F(ModernGaugeTest, ConfigurationAndInputValidation) {
  auto gauge = std::make_unique<ModernRadialGauge>(app_->context());
  EXPECT_EQ(gauge->style(), ModernRadialGauge::Style::kPrimaryContainer);
  EXPECT_FLOAT_EQ(gauge->value(), 0);
  EXPECT_FLOAT_EQ(gauge->startAngle(), -kPi / 2);
  EXPECT_FLOAT_EQ(gauge->endAngle(), kPi / 2);
  EXPECT_EQ(gauge->scaleWidth(), SmallNumber(8));
  EXPECT_EQ(gauge->valueWidth(), SmallNumber(4));
  EXPECT_TRUE(gauge->setValue(-1));
  EXPECT_FLOAT_EQ(gauge->value(), 0);
  EXPECT_TRUE(gauge->setValue(2));
  EXPECT_FLOAT_EQ(gauge->value(), 1);
  EXPECT_FALSE(gauge->setValue(std::numeric_limits<float>::quiet_NaN()));
  EXPECT_FALSE(gauge->setValue(std::numeric_limits<float>::infinity()));
  EXPECT_FLOAT_EQ(gauge->value(), 1);
  EXPECT_FALSE(gauge->setAngles(0, kTurn + 0.1f));
  EXPECT_FALSE(gauge->setAngles(0, std::numeric_limits<float>::infinity()));
  EXPECT_FLOAT_EQ(gauge->startAngle(), -kPi / 2);
  EXPECT_TRUE(gauge->setAngles(0, -kTurn));
  gauge->setScaleWidth(4.5f);
  gauge->setValueWidth(2.25f);
  EXPECT_EQ(gauge->scaleWidth(), SmallNumber(4.5f));
  EXPECT_EQ(gauge->valueWidth(), SmallNumber(2.25f));
  install(std::move(gauge));
  render();
  gauge_->setScaleWidth(0);
  gauge_->setValueWidth(0);
  render();
  EXPECT_EQ(count(roo_display::color::White), 160 * 128);
  gauge_->setScaleWidth(8);
  EXPECT_TRUE(gauge_->setAngles(1, 1));
  render();
  EXPECT_EQ(count(roo_display::color::White), 160 * 128);
}

// Verifies all Material 3 color pairs and explicit custom colors, with virtual
// hooks resolving colors from the current value rather than retained paint
// state.
TEST_F(ModernGaugeTest, StylesAndVirtualColorHooks) {
  ModernRadialGauge gauge(app_->context());
  const auto& colors = gauge.theme().material3Theme().color;
  using Style = ModernRadialGauge::Style;
  struct Expected {
    Style style;
    Color scale;
    Color value;
  };
  for (const auto& pair :
       {Expected{Style::kPrimaryContainer, colors.primaryContainer,
                 colors.onPrimaryContainer},
        Expected{Style::kSecondaryContainer, colors.secondaryContainer,
                 colors.onSecondaryContainer},
        Expected{Style::kTertiaryContainer, colors.tertiaryContainer,
                 colors.onTertiaryContainer},
        Expected{Style::kErrorContainer, colors.errorContainer,
                 colors.onErrorContainer}}) {
    gauge.setStyle(pair.style);
    EXPECT_EQ(gauge.getScaleColor(), pair.scale);
    EXPECT_EQ(gauge.getValueColor(), pair.value);
  }
  gauge.setCustomColors(roo_display::color::Blue, roo_display::color::Red);
  EXPECT_EQ(gauge.style(), Style::kCustom);
  EXPECT_EQ(gauge.getScaleColor(), roo_display::color::Blue);
  EXPECT_EQ(gauge.getValueColor(), roo_display::color::Red);

  auto dynamic = std::make_unique<ValueColoredGauge>(app_->context());
  dynamic->setAngles(0, kTurn);
  install(std::move(dynamic));
  gauge_->setValue(0.25f);
  render();
  EXPECT_GT(count(roo_display::color::Green), 0);
  EXPECT_GT(count(roo_display::color::Blue), 0);
  gauge_->setValue(0.75f);
  render();
  EXPECT_EQ(count(roo_display::color::Green), 0);
  EXPECT_EQ(count(roo_display::color::Blue), 0);
  EXPECT_GT(count(roo_display::color::Red), 0);
  EXPECT_GT(count(roo_display::color::Yellow), 0);
}

// Verifies zero and full values, the shared radius, mathematical angle origin
// and sweep direction using cardinal samples independent of raster internals.
TEST_F(ModernGaugeTest, ArcPositionsAndSignedSweeps) {
  auto gauge = std::make_unique<ModernRadialGauge>(app_->context());
  gauge->setCustomColors(roo_display::color::Blue, roo_display::color::Red);
  gauge->setAngles(0, kTurn);
  install(std::move(gauge));
  render();
  EXPECT_EQ(count(roo_display::color::Red), 0);
  for (float angle : {0.0f, kPi / 2, kPi, -kPi / 2})
    EXPECT_EQ(atAngle(angle), roo_display::color::Blue);
  gauge_->setValue(0.25f);
  render();
  EXPECT_EQ(atAngle(0), roo_display::color::Red);
  EXPECT_EQ(atAngle(kPi / 2), roo_display::color::Red);
  EXPECT_EQ(atAngle(kPi), roo_display::color::Blue);
  EXPECT_EQ(atAngle(-kPi / 2), roo_display::color::Blue);
  gauge_->setAngles(0, -kTurn);
  render();
  EXPECT_EQ(atAngle(-kPi / 2), roo_display::color::Red);
  EXPECT_EQ(atAngle(kPi / 2), roo_display::color::Blue);
  gauge_->setValue(1);
  render();
  for (float angle : {0.0f, kPi / 2, kPi, -kPi / 2})
    EXPECT_EQ(atAngle(angle), roo_display::color::Red);
  // The thinner foreground leaves the scale visible along both stroke edges.
  EXPECT_GT(count(roo_display::color::Blue), 0);
}

// Verifies the default semicircle is oriented right, with a bottom start and
// top end; bounds remain empty outside the actual arc and its round caps.
TEST_F(ModernGaugeTest, DefaultSemicircleAndRoundCaps) {
  auto gauge = std::make_unique<ModernRadialGauge>(app_->context(), 0.5f);
  gauge->setCustomColors(roo_display::color::Blue, roo_display::color::Red);
  install(std::move(gauge));
  render();
  EXPECT_GT(count(roo_display::color::Blue), 0);
  EXPECT_GT(count(roo_display::color::Red), 0);
  EXPECT_EQ(pixel(10, 15), roo_display::color::White);
  EXPECT_EQ(pixel(10, 114), roo_display::color::White);
  // Fitted centerline: center=(36.5,64.5), radius=46 at 75% zoom.
  float stroke = Scaled(static_cast<float>(gauge_->scaleWidth()));
  float radius = (100 - stroke - 2) * 0.5f;
  float center_x = 10 + (99 - radius) * 0.5f;
  EXPECT_EQ(pixel(std::lround(center_x), std::lround(64.5f + radius)),
            roo_display::color::Red);
  EXPECT_EQ(pixel(std::lround(center_x), std::lround(64.5f - radius)),
            roo_display::color::Blue);
  EXPECT_EQ(pixel(std::lround(center_x), 65), roo_display::color::White);
}

// Verifies changing values, widths and angles reconstructs the same final
// pixels as a fresh draw, preserving a patterned backdrop and once-only writes
// in turbo.
TEST_F(ModernGaugeTest, UpdatesAndMenuReturnMatchFreshFrames) {
  for (bool turbo : {false, true}) {
    if (turbo) display_.enableTurbo();
    if (gauge_ == nullptr) {
      auto gauge = std::make_unique<ModernRadialGauge>(app_->context());
      gauge->setCustomColors(roo_display::color::Blue, roo_display::color::Red);
      install(std::move(gauge), false, true);
    }
    for (float value : {0.0f, 0.001f, 0.2f, 0.7f, 1.0f, 0.1f}) {
      SCOPED_TRACE(value);
      gauge_->setValue(value);
      render();
      auto incremental = snapshot();
      render(true);
      EXPECT_EQ(incremental, snapshot());
    }
    gauge_->setScaleWidth(12.5f);
    gauge_->setValueWidth(8.25f);
    gauge_->setAngles(kPi, 0);
    render();
    auto resized = snapshot();
    render(true);
    EXPECT_EQ(resized, snapshot());

    Backdrop cover(app_->context());
    task_->navigation().push(cover);
    render();
    gauge_->setValue(0.85f);
    task_->navigation().pop();
    render();
    auto returned = snapshot();
    render(true);
    EXPECT_EQ(returned, snapshot());
  }
}
// Verifies translucent arcs blend in foreground-over-scale order against the
// ancestor in one pass, and that either stroke may be independently wider.
TEST_F(ModernGaugeTest, AlphaCompositionAndWiderValueStroke) {
  auto gauge = std::make_unique<ModernRadialGauge>(app_->context(), 1);
  Color scale(128, 0, 0, 255);
  Color foreground(128, 255, 0, 0);
  gauge->setCustomColors(scale, foreground);
  gauge->setAngles(0, kTurn);
  install(std::move(gauge));
  render();
  Color expected = roo_display::AlphaBlend(
      roo_display::AlphaBlend(roo_display::color::White, scale), foreground);
  roo_display::Argb4444 format;
  expected = format.toArgbColor(format.fromArgbColor(expected));
  EXPECT_EQ(atAngle(0), expected);
  EXPECT_EQ(pixel(60, 65), roo_display::color::White);
  gauge_->setCustomColors(roo_display::color::Blue, roo_display::color::Red);
  gauge_->setScaleWidth(4.25f);
  gauge_->setValueWidth(12.5f);
  render();
  EXPECT_EQ(atAngle(0), roo_display::color::Red);
  EXPECT_EQ(count(roo_display::color::Blue), 0);
  auto wide = snapshot();
  render(true);
  EXPECT_EQ(wide, snapshot());
}

// Verifies value changes keep the dirty/invalidated distinction, coalesce
// several assignments against the last painted value, and touch only a small
// endpoint area while preserving alpha and patterned backdrop composition.
TEST_F(ModernGaugeTest, SmallDirtyUpdatesPreserveUnchangedPixels) {
  auto gauge = std::make_unique<ModernRadialGauge>(app_->context(), 0.4f);
  gauge->setCustomColors(Color(128, 0, 0, 255), Color(160, 255, 0, 0));
  gauge->setAngles(0, kTurn);
  install(std::move(gauge), false, true);
  render();
  for (float next : {0.41f, 0.39f, 0.42f}) {
    gauge_->setValue(next + 0.1f);
    gauge_->setValue(next);
    EXPECT_TRUE(gauge_->isDirty());
    EXPECT_FALSE(gauge_->isInvalidated());
    render();
    int written = std::count(device_.writes.begin(), device_.writes.end(), 1);
    EXPECT_GT(written, 0);
    EXPECT_LT(written, 500);
    auto incremental = snapshot();
    render(true);
    EXPECT_EQ(incremental, snapshot());
  }
}

}  // namespace
