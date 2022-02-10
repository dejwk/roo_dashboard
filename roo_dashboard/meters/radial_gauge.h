#include <cmath>

#include "roo_display.h"
#include "roo_display/core/offscreen.h"
#include "roo_display/font/font.h"
#include "roo_display/shape/basic_shapes.h"
#include "roo_display/ui/color_gradient.h"
#include "roo_display/ui/string_printer.h"
#include "roo_display/ui/text_label.h"
#include "roo_smooth_fonts/NotoSans_Condensed/15.h"
#include "roo_windows/core/widget.h"

namespace roo_dashboard {

inline roo_display::Color colorForValue(float value) {
  static roo_display::ColorGradient gradient(
      {std::make_pair(0.0, roo_display::color::Red),
       std::make_pair(40.0, roo_display::color::White),
       std::make_pair(100.0, roo_display::color::White)});
  return gradient.getColor(value);
};

class RadialGauge : public roo_windows::Widget {
 public:
  struct Spec {
    roo_display::Box extents;
    int16_t x_center;
    int16_t y_center;
    int16_t radius;
    int16_t scale_width;
    float min_scale_value;
    float max_scale_value;
    float divider_spacing;
    int16_t ticks_per_divider;
    float deg_scale_start;
    float deg_scale_end;
    float deg_needle_start;
    float deg_needle_end;
    std::function<roo_display::Color(float)> scale_color;
    int16_t face_x_offset;
    int16_t face_y_offset;
  };

  RadialGauge(const roo_windows::Environment& env, float value = 0)
      : Widget(env),
        spec_{.extents = roo_display::Box(0, 50, 310, 200),
              .x_center = 160,
              .y_center = 240,
              .radius = 130,
              .scale_width = 20,
              .min_scale_value = 0,
              .max_scale_value = 100,
              .divider_spacing = 20,
              .ticks_per_divider = 5,
              .deg_scale_start = -50,
              .deg_scale_end = 50,
              .deg_needle_start = -55,
              .deg_needle_end = 55,
              .scale_color = &colorForValue,
              .face_x_offset = 0,
              .face_y_offset = -80},
        current_value_(value),
        previous_value_(value) {}

  roo_windows::Dimensions getSuggestedMinimumDimensions() const override;

  bool paint(const roo_display::Surface& s) override;

  void setValue(float value);

  void setFace(const roo_display::Drawable* face);
  void setBounds(const roo_display::Box& bounds);
  void setCenter(int16_t x, int16_t y);
  void setRangeAngles(float deg_scale_start, float deg_scale_end);
  void setRangeAngles(float deg_scale_start, float deg_scale_end,
                      float deg_needle_start, float deg_needle_end);

  // Sets the 'inner' radius of the scale.
  void setScaleRadius(float radius);

  // Sets the width of the colored band of the scale, in pixels.
  void setScaleWidth(int16_t width);

  void setValueRange(float min_scale_value, float max_scale_value);

  void setDividers(float spacing, int16_t subdivision);

  void setScaleColoring(std::function<roo_display::Color(float)> coloring);

 private:
  float currentDeg() const;
  float previousDeg() const;

  Spec spec_;
  const roo_display::Drawable* face_;

  float current_value_;
  float previous_value_;
};

}  // namespace roo_dashboard