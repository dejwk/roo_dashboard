#pragma once

#include <cmath>

#include "roo_display/ui/color_gradient.h"
#include "roo_windows/core/panel.h"
#include "roo_windows/core/widget.h"
#include "roo_windows/widgets/text_label.h"

namespace roo_dashboard {

// Analog thermometer, with range suitable for measuring indoor or swimming pool
// temperatures (~10 - 33 deg C).
class Thermometer : public roo_windows::Panel {
 public:
  class Indicator : public roo_windows::Widget {
   public:
    Indicator(const roo_windows::Environment& env,
              const roo_display::ColorGradient& temperature_gradient)
        : roo_windows::Widget(env),
          temperature_gradient_(temperature_gradient) {
      setTemperature(std::nanf(""));
    }

    roo_windows::Dimensions getSuggestedMinimumDimensions() const override {
      return roo_windows::Dimensions(56, 232);
    }

    void paint(const roo_windows::Canvas& canvas) const override;

    void setTemperature(float tempC);

   private:
    const roo_display::ColorGradient& temperature_gradient_;
    int16_t temp_height_pixels_;
    roo_display::Color temp_color_;
  };

  Thermometer(const roo_windows::Environment& env);

  Thermometer(const roo_windows::Environment& env,
              const roo_display::ColorGradient& temp_gradient);

  void setTemperature(float tempC);

  roo_windows::Dimensions onMeasure(roo_windows::WidthSpec width,
                                    roo_windows::HeightSpec height) override;

  void onLayout(bool changed, const roo_windows::Rect& rect) override;

 private:
  Indicator indicator_;
  roo_windows::TextLabel caption_;

  float tempC_;
};

}  // namespace roo_dashboard