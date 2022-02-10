#include "thermometer.h"

#include "roo_dashboard/meters/resources/thermometer_246x80_bar.h"
#include "roo_dashboard/meters/resources/thermometer_246x80_bounds.h"
#include "roo_display/core/color.h"
#include "roo_display/shape/basic_shapes.h"
#include "roo_display/ui/string_printer.h"
#include "roo_smooth_fonts/NotoSans_Regular/27.h"

using namespace roo_display;
using namespace roo_windows;

namespace roo_dashboard {

namespace {
// Default gradient for the thermometer.
const roo_display::ColorGradient& getDefaultColorGradient() {
  static roo_display::ColorGradient gradient({
      std::make_pair(0.0, Color(0, 0, 0)),         // Black.
      std::make_pair(12.0, Color(94, 94, 255)),    // Purplish blue.
      std::make_pair(22.0, Color(153, 195, 255)),  // Light blue.
      std::make_pair(23.0, Color(255, 207, 94)),   // Yellow.
      std::make_pair(30.0, Color(255, 31, 31)),    // Red.
      std::make_pair(40.0, Color(133, 0, 0)),      // Dark red.
  });
  return gradient;
}
}  // namespace

void Thermometer::Indicator::setTemperature(float tempC) {
  if (std::isnan(tempC)) {
    temp_height_pixels_ = 124;
    setEnabled(false);
    return;
  }
  // If we were nan (disabled) before, this will also make us dirty.
  setEnabled(true);
  // We have 20°C at height 124.
  int16_t new_height_pixels = 124 - (int)((tempC - 20.0) * 10);
  if (new_height_pixels != temp_height_pixels_) {
    temp_height_pixels_ = new_height_pixels;
    temp_color_ = temperature_gradient_.getColor(tempC);
    markDirty();
  }
}

bool Thermometer::Indicator::paint(const Surface& s) {
  // For now, we use a fixed range.
  if (isInvalidated()) {
    s.drawObject(thermometer_246x80_bounds());
    // Draw ticks.
    for (int i = 0; i < 33; i++) {
      int y = (i * 5 + 15);
      int width = 3;
      if ((i - 4) % 20 == 0) {
        // Full tens.
        width = 10;
      } else if ((i - 4) % 10 == 0) {
        width = 7;
      } else if ((i - 4) % 2 == 0) {
        width = 5;
      }
      s.drawObject(FilledRect(46, y, 46 + width - 1, y, color::Black));
      s.drawObject(
          FilledRect(46, y + 1, 46 + width - 1, y + 1, color::LightGray));
    }
  }

  Surface news = s;
  news.set_dx(s.dx() + 10);
  news.set_dy(s.dy() + 10);
  news.set_fill_mode(roo_display::FILL_MODE_VISIBLE);

  typedef RleImage4bppxPolarized<Alpha4, PrgMemResource> Img;
  const Img& img = thermometer_246x80_bar();
  Img top(img.extents(), img.resource(), Alpha4(theme().color.background));
  Img bottom(img.extents(), img.resource(), Alpha4(temp_color_));

  Box clip = news.clip_box();

  news.set_clip_box(Box::intersect(
      clip,
      Box(0, 0, 57, temp_height_pixels_ - 1).translate(news.dx(), news.dy())));
  news.drawObject(top);

  news.set_clip_box(Box::intersect(
      clip,
      Box(0, temp_height_pixels_, 57, 500).translate(news.dx(), news.dy())));
  news.drawObject(bottom);
  return true;
}

Thermometer::Thermometer(const roo_windows::Environment& env)
    : Thermometer(env, getDefaultColorGradient()) {}

Thermometer::Thermometer(const roo_windows::Environment& env,
                         const roo_display::ColorGradient& temp_gradient)
    : roo_windows::Panel(env),
      indicator_(env, temp_gradient),
      caption_(env, "", font_NotoSans_Regular_27(), HAlign::Center(),
               VAlign::Top()) {
  add(&indicator_);
  add(&caption_);
  indicator_.setEnabled(false);
  caption_.setVisible(false);
}

void Thermometer::setTemperature(float tempC) {
  if (tempC_ == tempC || (std::isnan(tempC_) && std::isnan(tempC))) return;
  tempC_ = tempC;
  indicator_.setTemperature(tempC);
  caption_.setContent(StringPrinter::sprintf("%.1f°C", tempC_));
  caption_.setVisible(!std::isnan(tempC_));
}

Dimensions Thermometer::onMeasure(MeasureSpec width, MeasureSpec height) {
  indicator_.measure(MeasureSpec::Unspecified(58),
                     MeasureSpec::Unspecified(232));
  caption_.measure(MeasureSpec::Unspecified(98), MeasureSpec::Unspecified(40));
  Dimensions preferred(98, 280);
  return Dimensions(width.resolveSize(preferred.width()),
                    height.resolveSize(preferred.height()));
}

void Thermometer::onLayout(bool changed, const roo_display::Box& box) {
  // Pic's dimensions are 58x232. Center it horizontally, and align to top.
  int16_t xMinPic = (box.width() - 58) / 2;
  indicator_.layout(Box(xMinPic, 0, xMinPic + 57, 231));
  // We need 98x40 for the label.
  int16_t xMinLabel = (box.width() - 98) / 2;
  caption_.layout(Box(xMinLabel, 240, xMinLabel + 98, 279));
}

}  // namespace roo_dashboard
