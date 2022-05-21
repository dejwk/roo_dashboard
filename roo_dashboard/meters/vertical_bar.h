#pragma once

#include <functional>
#include <string>

#include "roo_windows/core/panel.h"
#include "roo_windows/core/preferred_size.h"
#include "roo_windows/core/widget.h"
#include "roo_windows/widgets/text_label.h"

namespace roo_dashboard {

class VerticalBar : public roo_windows::Panel {
 public:
  class Indicator : public roo_windows::Widget {
   public:
    Indicator(const roo_windows::Environment& env, float scale,
              int16_t zero_offset,
              std::function<roo_display::Color(float val)> color_fn,
              float initial_value)
        : roo_windows::Widget(env),
          scale_(scale),
          zero_offset_(zero_offset),
          color_fn_(color_fn),
          value_(-1),
          color_(roo_display::color::Transparent) {
      setValue(initial_value);
    }

    roo_windows::Dimensions getSuggestedMinimumDimensions() const override {
      return roo_windows::Dimensions(50, 10);
    }

    roo_windows::PreferredSize getPreferredSize() const override {
      using roo_windows::PreferredSize;
      return PreferredSize(PreferredSize::MatchParent(),
                           PreferredSize::WrapContent());
    }

    bool paint(const roo_display::Surface& s) override;

    void setValue(float value);

    int16_t zero_offset() const { return zero_offset_; }
    float scale() const { return scale_; }

   private:
    float scale_;
    int16_t zero_offset_;
    std::function<roo_display::Color(float val)> color_fn_;

    int16_t value_;
    roo_display::Color color_;

    roo_display::Color previous_color_;
    int16_t previous_value_;
  };

  VerticalBar(const roo_windows::Environment& env, float scale,
              int16_t zero_offset,
              std::function<roo_display::Color(float val)> color_fn,
              const std::string& title, const std::string& caption_template,
              float initial_value = 0.0);

  void setValue(float value);

  roo_windows::Dimensions onMeasure(roo_windows::MeasureSpec width,
                                    roo_windows::MeasureSpec height) override;

  void onLayout(bool changed, const roo_display::Box& box) override;

 private:
  roo_windows::TextLabel title_;
  Indicator indicator_;
  roo_windows::TextLabel caption_;

  const std::string caption_template_;

  float value_;
};

}  // namespace roo_dashboard
